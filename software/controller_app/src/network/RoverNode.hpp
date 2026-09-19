#pragma once
#include <QObject>
#include <QString>
#include <QDateTime>
#include "../core/Types.hpp"

class RoverNode : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString ip READ ip CONSTANT)
    Q_PROPERTY(QString boardName READ boardName NOTIFY updated)
    Q_PROPERTY(QString fwVersion READ fwVersion NOTIFY updated)
    Q_PROPERTY(int boardType READ boardType NOTIFY updated)
    Q_PROPERTY(float battery READ battery NOTIFY updated)
    Q_PROPERTY(bool connected READ connected NOTIFY updated)
    Q_PROPERTY(int rttMs READ rttMs NOTIFY updated)
    Q_PROPERTY(QString statusText READ statusText NOTIFY updated)
    // statusText: "CONNECTED" | "BEACON ONLY" | "STALE" | "DISCONNECTED"
    Q_PROPERTY(bool versionMismatch READ versionMismatch NOTIFY updated)

public:
    explicit RoverNode(const QString& ip, QObject* parent = nullptr);

    QString ip() const { return m_ip; }
    QString boardName() const { return m_boardName; }
    QString fwVersion() const { return m_fwVersion; }
    int boardType() const { return m_boardType; }
    float battery() const { return m_battery; }
    bool connected() const { return m_connected; }
    int rttMs() const { return m_rttMs; }
    QString statusText() const;
    bool versionMismatch() const { return m_versionMismatch; }

    // Last received full telemetry (for delegation from TelemetryClient)
    const VehicleTelemetryPacket& telemetry() const { return m_telemetry; }
    
    float relativeAltitudeM() const;

    void updateFromBeacon(const LbpBeaconPacket& bcn, const QString& senderIp);
    void updateFromTelemetry(const VehicleTelemetryPacket& tel);
    void updateFromPong(const LbpPongPacket& pong, qint64 pingTimeSentMs);
    void checkHealth(); // Called by watchdog - updates connected/status

    qint64 lastSeenMs() const { return m_lastSeenMs; }  // ms since epoch

signals:
    void updated();

private:
    QString m_ip;
    QString m_boardName = "Unknown";
    QString m_fwVersion = "?.?.?";
    int     m_boardType = BOARD_UNKNOWN;
    float   m_battery   = 0.0f;
    bool    m_connected = false;
    int     m_rttMs     = -1;
    bool    m_versionMismatch = false;

    qint64 m_lastSeenMs      = 0; // last beacon OR telemetry
    qint64 m_lastTelemetryMs = 0;
    qint64 m_lastBeaconMs    = 0;

    VehicleTelemetryPacket m_telemetry;
    bool m_hasTelemetry = false;
    float m_basePressurePa = 0.0f;

    static constexpr qint64 CONNECTED_TIMEOUT_MS = 5000;  // 5s no tel = disconnected
    static constexpr qint64 STALE_TIMEOUT_MS     = 30000; // 30s no anything = stale
};
