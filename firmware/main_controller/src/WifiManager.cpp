#include "WifiManager.hpp"
#include <Arduino.h>
#include <string.h>

// =============================================================================
// WifiManager — LBP v2 implementation for ESP32 main_controller
// =============================================================================

void WifiManager::begin(const char* ssid, const char* password) {
    m_ssid = ssid;
    m_pass = password;
    tryConnect();
}

void WifiManager::setBeaconInfo(uint8_t boardType, uint8_t fwMajor, uint8_t fwMinor,
                                uint8_t fwPatch, uint8_t hwRev, const char* boardName) {
    memset(&m_beacon, 0, sizeof(m_beacon));
    m_beacon.preamble        = LBP_PREAMBLE_BCN;
    m_beacon.protocolVersion = LBP_PROTOCOL_VERSION;
    m_beacon.boardType       = boardType;
    m_beacon.fwMajor         = fwMajor;
    m_beacon.fwMinor         = fwMinor;
    m_beacon.fwPatch         = fwPatch;
    m_beacon.hardwareRev     = hwRev;
    m_beacon.telemetrySize   = sizeof(VehicleTelemetryPacket);
    m_beacon.commandSize     = sizeof(VehicleCommandPacket);
    strncpy(m_beacon.boardName, boardName, sizeof(m_beacon.boardName) - 1);
    // CRC is recomputed in broadcastBeacon() so uptimeMs is always current
}

void WifiManager::tryConnect() {
    Serial.printf("[WifiManager] Connecting to SSID: %s\n", m_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(m_ssid, m_pass);
}

void WifiManager::openSockets() {
    m_cmdSocket.begin(LBP_PORT_CMD);
    // m_txSocket uses an ephemeral OS-assigned port for sending only
    m_txSocket.begin(0);
    m_socketsOpen = true;
    Serial.printf("[WifiManager] Sockets open. IP: %s\n", WiFi.localIP().toString().c_str());
}

void WifiManager::update() {
    uint32_t now = millis();

    // 1. WiFi watchdog
    if (now - m_lastWifiCheckMs >= WIFI_CHECK_INTERVAL_MS) {
        m_lastWifiCheckMs = now;
        if (WiFi.status() != WL_CONNECTED) {
            if (m_socketsOpen) {
                m_cmdSocket.stop();
                m_txSocket.stop();
                m_socketsOpen = false;
                m_clientPort = 0; // Lost client too
            }
            if (m_wifiLostMs == 0) {
                m_wifiLostMs = now;
                Serial.println("[WifiManager] WiFi lost — attempting reconnect...");
                WiFi.reconnect();
            } else if (now - m_wifiLostMs >= WIFI_HARD_RESET_MS) {
                Serial.println("[WifiManager] Hard WiFi reset after prolonged loss");
                WiFi.disconnect(true);
                delay(100);
                m_wifiLostMs = 0;
                tryConnect();
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

    if (!isReady()) return;

    // 2. Parse incoming UDP on the command socket
    parseCmdSocket();

    // 3. Broadcast beacon at 1 Hz
    if (now - m_lastBeaconMs >= BEACON_INTERVAL_MS) {
        m_lastBeaconMs = now;
        broadcastBeacon();
    }
}

void WifiManager::parseCmdSocket() {
    int packetSize;
    while ((packetSize = m_cmdSocket.parsePacket()) > 0) {
        IPAddress from   = m_cmdSocket.remoteIP();
        uint16_t  fport  = m_cmdSocket.remotePort();

        if (packetSize == sizeof(VehicleCommandPacket)) {
            VehicleCommandPacket cmd;
            m_cmdSocket.read(reinterpret_cast<uint8_t*>(&cmd), sizeof(cmd));
            if (cmd.preamble == LBP_PREAMBLE_CMD && validateCrc(reinterpret_cast<uint8_t*>(&cmd), sizeof(cmd))) {
                m_clientIP   = from;
                m_clientPort = fport;
                m_lastCmdMs  = millis();
                m_pendingCmd = cmd;
                m_cmdPending = true;
            }
        } else if (packetSize == sizeof(LbpPingPacket)) {
            LbpPingPacket ping;
            m_cmdSocket.read(reinterpret_cast<uint8_t*>(&ping), sizeof(ping));
            if (ping.preamble == LBP_PREAMBLE_PING && validateCrc(reinterpret_cast<uint8_t*>(&ping), sizeof(ping))) {
                handlePing(ping, from, fport);
            }
        } else {
            // Unknown packet — flush it
            while (m_cmdSocket.available()) m_cmdSocket.read();
        }
    }
}

void WifiManager::handlePing(const LbpPingPacket& ping, IPAddress from, uint16_t port) {
    // A PING establishes/maintains the client session just like a CMD
    m_clientIP   = from;
    m_clientPort = port;
    m_lastCmdMs  = millis();

    LbpPongPacket pong;
    memset(&pong, 0, sizeof(pong));
    pong.preamble       = LBP_PREAMBLE_PONG;
    pong.seqId          = ping.seqId;
    pong.clientMs       = ping.clientMs;
    pong.roverUptimeMs  = millis();
    pong.batteryVoltage = 0.0f; // Filled by main.cpp after construction; 0 is safe default

    size_t dataLen = sizeof(LbpPongPacket) - sizeof(uint16_t);
    pong.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&pong), dataLen);

    m_txSocket.beginPacket(from, LBP_PORT_TEL);
    m_txSocket.write(reinterpret_cast<const uint8_t*>(&pong), sizeof(pong));
    m_txSocket.endPacket();
}

void WifiManager::broadcastBeacon() {
    m_beacon.uptimeMs = millis();
    size_t dataLen = sizeof(LbpBeaconPacket) - sizeof(uint16_t);
    m_beacon.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&m_beacon), dataLen);

    m_txSocket.beginPacket(IPAddress(255, 255, 255, 255), LBP_PORT_BEACON);
    m_txSocket.write(reinterpret_cast<const uint8_t*>(&m_beacon), sizeof(m_beacon));
    m_txSocket.endPacket();
}

void WifiManager::sendTelemetry(const VehicleTelemetryPacket& pkt) {
    if (!hasActiveClient()) return;
    m_txSocket.beginPacket(m_clientIP, LBP_PORT_TEL);
    m_txSocket.write(reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
    m_txSocket.endPacket();
}

bool WifiManager::readCommand(VehicleCommandPacket& out) {
    if (!m_cmdPending) return false;
    out = m_pendingCmd;
    m_cmdPending = false;
    return true;
}

bool WifiManager::isReady() const {
    return WiFi.status() == WL_CONNECTED && m_socketsOpen;
}

bool WifiManager::hasActiveClient() const {
    return m_clientPort != 0 && (millis() - m_lastCmdMs < CLIENT_TIMEOUT_MS);
}

// CRC-16-CCITT (init 0xFFFF, poly 0x1021)
uint16_t WifiManager::calculateCrc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int j = 0; j < 8; ++j)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

// Validates a packet: CRC of data[0..fullLen-3] must equal *(uint16_t*)(data+fullLen-2)
bool WifiManager::validateCrc(const uint8_t* data, size_t fullLen) {
    if (fullLen < 2) return false;
    uint16_t computed = calculateCrc16(data, fullLen - 2);
    uint16_t received;
    memcpy(&received, data + fullLen - 2, 2);
    return computed == received;
}
