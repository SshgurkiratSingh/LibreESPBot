#include "WifiManager.hpp"
#include <Arduino.h>
#include <cstring>

void WifiManager::begin(const char* ssid, const char* password) {
    strncpy(m_ssid, ssid, sizeof(m_ssid) - 1);
    strncpy(m_password, password, sizeof(m_password) - 1);
    memset(&m_beacon, 0, sizeof(m_beacon));
    
    // Register WiFi event handler to debug disconnects
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            Serial.printf("[WiFiEvent] Disconnected from station, reason: %d\n", info.wifi_sta_disconnected.reason);
        } else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
            Serial.println("[WiFiEvent] Connected to station");
        } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
            Serial.println("[WiFiEvent] Got IP address");
        }
    });

    tryConnect();
}

void WifiManager::setBeaconInfo(uint8_t boardType, uint8_t fwMajor, uint8_t fwMinor, uint8_t fwPatch, uint8_t hwRev, const char* boardName) {
    m_beacon.preamble = LBP_PREAMBLE_BCN;
    m_beacon.protocolVersion = LBP_PROTOCOL_VERSION;
    m_beacon.boardType = boardType;
    m_beacon.fwMajor = fwMajor;
    m_beacon.fwMinor = fwMinor;
    m_beacon.fwPatch = fwPatch;
    m_beacon.hardwareRev = hwRev;
    m_beacon.telemetrySize = sizeof(VehicleTelemetryPacket);
    m_beacon.commandSize = sizeof(VehicleCommandPacket);
    strncpy(m_beacon.boardName, boardName, sizeof(m_beacon.boardName) - 1);
}

void WifiManager::tryConnect() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(m_ssid, m_password);
}

void WifiManager::openSockets() {
    m_cmdSocket.begin(LBP_PORT_CMD); // 8888
    m_bcnSocket.begin(0); // Ephemeral
    m_socketsOpen = true;
}

void WifiManager::update() {
    uint32_t now = millis();
    
    // 1) WiFi watchdog
    if (now - m_lastWifiCheckMs >= WIFI_CHECK_INTERVAL_MS) {
        m_lastWifiCheckMs = now;
        if (WiFi.status() != WL_CONNECTED) {
            if (m_wifiLostMs == 0) {
                m_wifiLostMs = now;
            }
            if (now - m_wifiLostMs > WIFI_RECONNECT_HARD_MS) {
                Serial.println("[WifiManager] Hard WiFi reset after prolonged loss");
                WiFi.disconnect(true);
                delay(100);
                WiFi.begin(m_ssid, m_password);
                m_wifiLostMs = now;
            } else {
                Serial.println("[WifiManager] WiFi lost — attempting reconnect...");
                WiFi.reconnect();
            }
        } else {
            if (m_wifiLostMs != 0) {
                Serial.println("[WifiManager] WiFi reconnected!");
                m_wifiLostMs = 0;
            }
            if (!m_socketsOpen) {
                openSockets();
            }
        }
    }

    // 2) Parse commands
    if (isReady()) {
        parseCmdSocket();
    }

    // 3) Broadcast beacon
    if (isReady() && (now - m_lastBeaconMs >= BEACON_INTERVAL_MS)) {
        m_lastBeaconMs = now;
        broadcastBeacon();
    }
}

void WifiManager::parseCmdSocket() {
    int packetSize;
    while ((packetSize = m_cmdSocket.parsePacket()) > 0) {
        uint8_t buffer[sizeof(VehicleCommandPacket)]; // Large enough for cmd or ping
        if (packetSize > (int)sizeof(buffer)) {
            Serial.printf("[WifiManager] Dropping oversized packet: %d bytes\n", packetSize);
            m_cmdSocket.flush();
            continue;
        }
        
        m_cmdSocket.read(buffer, packetSize);
        
        if (packetSize == sizeof(VehicleCommandPacket)) {
            VehicleCommandPacket* pkt = (VehicleCommandPacket*)buffer;
            if (pkt->preamble == LBP_PREAMBLE_CMD) {
                if (validateCrc(buffer, packetSize)) {
                    memcpy(&m_pendingCmd, pkt, sizeof(VehicleCommandPacket));
                    m_cmdPending = true;
                    m_clientIP = m_cmdSocket.remoteIP();
                    m_clientPort = m_cmdSocket.remotePort();
                    m_lastCmdMs = millis();
                } else {
                    Serial.println("[WifiManager] CMD packet failed CRC!");
                }
            } else {
                Serial.printf("[WifiManager] Unknown 25-byte packet, preamble: %04X\n", pkt->preamble);
            }
        } else if (packetSize == sizeof(LbpPingPacket)) {
            LbpPingPacket* pkt = (LbpPingPacket*)buffer;
            if (pkt->preamble == LBP_PREAMBLE_PING) {
                if (validateCrc(buffer, packetSize)) {
                    handlePing(*pkt, m_cmdSocket.remoteIP(), m_cmdSocket.remotePort());
                } else {
                    Serial.println("[WifiManager] PING packet failed CRC!");
                }
            }
        } else {
            Serial.printf("[WifiManager] Unknown packet size: %d bytes\n", packetSize);
        }
    }
}

void WifiManager::handlePing(const LbpPingPacket& ping, IPAddress from, uint16_t port) {
    // A PING establishes/maintains the client session just like a CMD
    m_clientIP   = from;
    m_clientPort = port;
    m_lastCmdMs  = millis();

    LbpPongPacket pong;
    pong.preamble = LBP_PREAMBLE_PONG;
    pong.seqId = ping.seqId;
    pong.clientMs = ping.clientMs;
    pong.roverUptimeMs = millis();
    pong.batteryVoltage = 0.0f; // Could get from telemetry, but standard specifies using 0 if not available directly
    pong.crc16 = calculateCrc16((const uint8_t*)&pong, sizeof(pong) - 2);
    
    m_bcnSocket.beginPacket(from, LBP_PORT_TEL); // 8889
    m_bcnSocket.write((const uint8_t*)&pong, sizeof(pong));
    m_bcnSocket.endPacket();
}

void WifiManager::broadcastBeacon() {
    m_beacon.uptimeMs = millis();
    m_beacon.crc16 = calculateCrc16((const uint8_t*)&m_beacon, sizeof(m_beacon) - 2);
    
    m_bcnSocket.beginPacket(IPAddress(255, 255, 255, 255), LBP_PORT_BEACON);
    m_bcnSocket.write((const uint8_t*)&m_beacon, sizeof(m_beacon));
    m_bcnSocket.endPacket();
}

void WifiManager::sendTelemetry(const VehicleTelemetryPacket& pkt) {
    if (!hasActiveClient()) {
        return;
    }
    
    // Standard spec says use m_bcnSocket which is the sending-only socket
    m_bcnSocket.beginPacket(m_clientIP, LBP_PORT_TEL); // 8889
    m_bcnSocket.write((const uint8_t*)&pkt, sizeof(pkt));
    m_bcnSocket.endPacket();
}

bool WifiManager::readCommand(VehicleCommandPacket& out) {
    if (m_cmdPending) {
        out = m_pendingCmd;
        m_cmdPending = false;
        return true;
    }
    return false;
}

bool WifiManager::hasActiveClient() const {
    return m_clientPort != 0 && (millis() - m_lastCmdMs < CLIENT_TIMEOUT_MS);
}

bool WifiManager::isReady() const {
    return WiFi.status() == WL_CONNECTED && m_socketsOpen;
}

IPAddress WifiManager::clientIP() const {
    return m_clientIP;
}

uint16_t WifiManager::clientPort() const {
    return m_clientPort;
}

uint16_t WifiManager::calculateCrc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool WifiManager::validateCrc(const uint8_t* data, size_t fullLen) {
    if (fullLen < 2) return false;
    uint16_t computed = calculateCrc16(data, fullLen - 2);
    uint16_t expected = *(const uint16_t*)(data + fullLen - 2);
    return computed == expected;
}
