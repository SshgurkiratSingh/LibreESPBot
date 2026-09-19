#pragma once

#include <WiFi.h>
#include <WiFiUdp.h>
#include "../include/Types.hpp"

class WifiManager {
public:
    void begin(const char* ssid, const char* password);
    void update();
    bool isReady() const;
    bool hasActiveClient() const;
    IPAddress clientIP() const;
    uint16_t clientPort() const;
    bool readCommand(VehicleCommandPacket& out);
    void sendTelemetry(const VehicleTelemetryPacket& pkt);
    void setBeaconInfo(uint8_t boardType, uint8_t fwMajor, uint8_t fwMinor, uint8_t fwPatch, uint8_t hwRev, const char* boardName);

private:
    char m_ssid[64] = {0};
    char m_password[64] = {0};

    WiFiUDP m_cmdSocket;
    WiFiUDP m_telSocket;
    WiFiUDP m_bcnSocket;

    IPAddress m_clientIP;
    uint16_t m_clientPort = 0;

    uint32_t m_lastCmdMs = 0;
    uint32_t m_lastBeaconMs = 0;
    uint32_t m_lastWifiCheckMs = 0;
    uint32_t m_wifiLostMs = 0;
    bool m_socketsOpen = false;
    
    LbpBeaconPacket m_beacon;
    VehicleCommandPacket m_pendingCmd;
    bool m_cmdPending = false;

    static const uint32_t CLIENT_TIMEOUT_MS = 3000;
    static const uint32_t BEACON_INTERVAL_MS = 1000;
    static const uint32_t WIFI_CHECK_INTERVAL_MS = 5000;
    static const uint32_t WIFI_RECONNECT_HARD_MS = 30000;

    void tryConnect();
    void openSockets();
    void broadcastBeacon();
    void parseCmdSocket();
    uint16_t calculateCrc16(const uint8_t* data, size_t len);
    void handlePing(const LbpPingPacket& ping, IPAddress from, uint16_t port);
    bool validateCrc(const uint8_t* data, size_t fullLen);
};
