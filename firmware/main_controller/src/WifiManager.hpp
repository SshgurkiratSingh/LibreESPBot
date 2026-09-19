#pragma once
#include <WiFi.h>
#include <WiFiUdp.h>
#include "Types.hpp"

// =============================================================================
// WifiManager — LibreBot Protocol v2 network stack for ESP32 (main_controller)
// =============================================================================
class WifiManager {
public:
    // Call once in setup(). Does not block — WiFi connects asynchronously.
    void begin(const char* ssid, const char* password);

    // Must be called every loop() iteration. Drives all network logic.
    void update();

    // Set beacon identity fields — call once in setup() after begin().
    void setBeaconInfo(uint8_t boardType, uint8_t fwMajor, uint8_t fwMinor,
                       uint8_t fwPatch, uint8_t hwRev, const char* boardName);

    // Returns true when WiFi is up and UDP sockets are open.
    bool isReady() const;

    // Returns true if a valid CMD has been received within CLIENT_TIMEOUT_MS.
    bool hasActiveClient() const;

    IPAddress clientIP()   const { return m_clientIP; }
    uint16_t  clientPort() const { return m_clientPort; }

    // Returns true and fills `out` if a new validated command is available.
    bool readCommand(VehicleCommandPacket& out);

    // Sends telemetry to the active client on port LBP_PORT_TEL.
    // No-op if hasActiveClient() is false.
    void sendTelemetry(const VehicleTelemetryPacket& pkt);

private:
    void tryConnect();
    void openSockets();
    void broadcastBeacon();
    void parseCmdSocket();
    void handlePing(const LbpPingPacket& ping, IPAddress from, uint16_t port);

    uint16_t calculateCrc16(const uint8_t* data, size_t length);
    bool     validateCrc(const uint8_t* data, size_t fullLen);

    WiFiUDP m_cmdSocket;   // Bound to LBP_PORT_CMD (8888) — receives CMDs and pings
    WiFiUDP m_txSocket;    // Ephemeral port — sends telemetry and beacons

    IPAddress m_clientIP;
    uint16_t  m_clientPort    = 0;
    uint32_t  m_lastCmdMs     = 0;

    uint32_t  m_lastBeaconMs  = 0;
    uint32_t  m_lastWifiCheckMs = 0;
    uint32_t  m_wifiLostMs    = 0;   // When WiFi was first seen as gone
    bool      m_socketsOpen   = false;

    LbpBeaconPacket     m_beacon{};
    VehicleCommandPacket m_pendingCmd{};
    bool                m_cmdPending = false;

    const char* m_ssid = nullptr;
    const char* m_pass = nullptr;

    // Tunables
    static constexpr uint32_t CLIENT_TIMEOUT_MS      = 3000;
    static constexpr uint32_t BEACON_INTERVAL_MS     = 1000;
    static constexpr uint32_t WIFI_CHECK_INTERVAL_MS = 5000;
    static constexpr uint32_t WIFI_HARD_RESET_MS     = 30000; // full re-begin after this
};
