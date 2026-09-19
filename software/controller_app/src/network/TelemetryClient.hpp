#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QVariantList>
#include "NodeRegistry.hpp"
#include "../core/Types.hpp"

class TelemetryClient : public QObject {
    Q_OBJECT
    // All data properties now delegate to the active node.
    // These are kept for QML backwards-compatibility.
    Q_PROPERTY(bool connected READ connected NOTIFY connectionStateChanged)
    Q_PROPERTY(float pitch READ pitch NOTIFY telemetryUpdated)
    Q_PROPERTY(float roll READ roll NOTIFY telemetryUpdated)
    Q_PROPERTY(float yaw READ yaw NOTIFY telemetryUpdated)
    Q_PROPERTY(float headingCompassDeg READ headingCompassDeg NOTIFY telemetryUpdated)
    Q_PROPERTY(float batteryVoltage READ batteryVoltage NOTIFY telemetryUpdated)
    Q_PROPERTY(float imuTempC READ imuTempC NOTIFY telemetryUpdated)
    Q_PROPERTY(float baroTempC READ baroTempC NOTIFY telemetryUpdated)
    Q_PROPERTY(float baroPressurePa READ baroPressurePa NOTIFY telemetryUpdated)
    Q_PROPERTY(float relativeAltitudeM READ relativeAltitudeM NOTIFY telemetryUpdated)
    Q_PROPERTY(int irArrayState READ irArrayState NOTIFY telemetryUpdated)
    Q_PROPERTY(int tof1DistMm READ tof1DistMm NOTIFY telemetryUpdated)
    Q_PROPERTY(int tof2DistMm READ tof2DistMm NOTIFY telemetryUpdated)
    Q_PROPERTY(int servoAngleDeg READ servoAngleDeg NOTIFY telemetryUpdated)
    Q_PROPERTY(float linearAccX READ linearAccX NOTIFY telemetryUpdated)
    Q_PROPERTY(float linearAccY READ linearAccY NOTIFY telemetryUpdated)
    Q_PROPERTY(float linearAccZ READ linearAccZ NOTIFY telemetryUpdated)
    Q_PROPERTY(int motorLeftPwm READ motorLeftPwm NOTIFY telemetryUpdated)
    Q_PROPERTY(int motorRightPwm READ motorRightPwm NOTIFY telemetryUpdated)
    Q_PROPERTY(int activeImuType READ activeImuType NOTIFY telemetryUpdated)
    Q_PROPERTY(int activeMagType READ activeMagType NOTIFY telemetryUpdated)
    Q_PROPERTY(int statusFlags READ statusFlags NOTIFY telemetryUpdated)
    Q_PROPERTY(QString firmwareVersion READ firmwareVersion NOTIFY telemetryUpdated)
    Q_PROPERTY(QString boardName READ boardName NOTIFY telemetryUpdated)
    Q_PROPERTY(bool versionMismatch READ versionMismatch NOTIFY telemetryUpdated)

public:
    explicit TelemetryClient(NodeRegistry* registry, QObject* parent = nullptr);
    ~TelemetryClient();

    void startListening(quint16 port = LBP_PORT_TEL);
    QUdpSocket* socket() const { return m_socket; }

    // Connection state — true only if the active node has telemetry and is connected
    bool connected() const;

    // Telemetry accessors — all delegate to activeNode()->telemetry()
    float pitch()            const;
    float roll()             const;
    float yaw()              const;
    float headingCompassDeg()const;
    float batteryVoltage()   const;
    float imuTempC()         const;
    float baroTempC()        const;
    float baroPressurePa()   const;
    float relativeAltitudeM()const;
    int   irArrayState()     const;
    int   tof1DistMm()       const;
    int   tof2DistMm()       const;
    int   servoAngleDeg()    const;
    float linearAccX()       const;
    float linearAccY()       const;
    float linearAccZ()       const;
    int   motorLeftPwm()     const;
    int   motorRightPwm()    const;
    int   activeImuType()    const;
    int   activeMagType()    const;
    int   statusFlags()      const;
    QString firmwareVersion()const;
    QString boardName()      const;
    bool  versionMismatch()  const;

    // Ping support — CommandEmitter calls this to track RTT per-node
    void recordPingSent(const QString& ip, uint16_t seqId, qint64 timestampMs);

signals:
    void telemetryUpdated();
    void connectionStateChanged();
    void connectionLost();

private slots:
    void readPendingDatagrams();
    void checkConnectionHealth();

private:
    uint16_t calculateCrc16(const uint8_t* data, size_t length);

    NodeRegistry*  m_registry;
    QUdpSocket*    m_socket;
    QTimer*        m_watchdogTimer;

    bool m_wasConnected = false;

    // Ping tracking: seqId -> {ip, timestampMs}
    struct PingRecord { QString ip; qint64 sentMs; };
    QMap<uint16_t, PingRecord> m_pendingPings;
};
