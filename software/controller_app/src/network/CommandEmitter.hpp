#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include "../core/Types.hpp"

class NodeRegistry;
class TelemetryClient;

class CommandEmitter : public QObject {
    Q_OBJECT
    Q_PROPERTY(int currentThrottle READ currentThrottle NOTIFY currentThrottleChanged)
    Q_PROPERTY(int currentSteering READ currentSteering NOTIFY currentSteeringChanged)
    Q_PROPERTY(int currentSpeedMode READ currentSpeedMode NOTIFY currentSpeedModeChanged)

public:
    explicit CommandEmitter(NodeRegistry* registry, QObject* parent = nullptr);
    ~CommandEmitter();

    void setTelemetryClient(TelemetryClient* tc) { m_telemetryClient = tc; }

public slots:
    void setTargetAddress(const QString& ip, quint16 port = LBP_PORT_CMD);
    void setSharedSocket(QUdpSocket* socket);
    void startEmitting(int intervalMs = 20); // 50 Hz
    void stopEmitting();

    int currentThrottle() const { return m_packet.throttleAxis; }
    int currentSteering() const { return m_packet.steeringAxis; }
    int currentSpeedMode() const { return m_packet.speedModeLimit; }

signals:
    void currentThrottleChanged();
    void currentSteeringChanged();
    void currentSpeedModeChanged();

public slots:   // Input hooks for the UI/Gamepad
    void updateThrottle(int throttle);
    void updateSteering(int steering);
    void setAutoBrake(bool enable);
    void setApfAvoidance(bool enable);
    void setRadarSweep(bool enable);
    void setRadarSweepSpeed(uint8_t speed);
    void setNoLagMode(bool enable);
    void setSpeedMode(uint8_t mode);
    void setHeadlightMode(int mode);
    void setCustomLedColor(int r, int g, int b);
    void setCustomLedPattern(uint8_t pattern);
    

    void setAutoTurn(bool enable, float targetHeading);

private slots:
    void sendCommandPacket();
    void sendPing();          // Called every 1s to measure RTT

private:
    uint16_t calculateCrc16(const uint8_t *data, size_t length);

    NodeRegistry*    m_registry;
    TelemetryClient* m_telemetryClient = nullptr;
    QUdpSocket*      m_socket;
    bool             m_ownsSocket;
    QTimer*          m_timer;
    QTimer*          m_pingTimer;
    QHostAddress     m_targetIp;
    quint16          m_targetPort;
    uint16_t         m_pingSeqId = 0;

    VehicleCommandPacket m_packet;
};
