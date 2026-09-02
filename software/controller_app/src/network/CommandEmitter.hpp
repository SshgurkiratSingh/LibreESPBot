#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include "../core/Types.hpp"

class CommandEmitter : public QObject {
    Q_OBJECT
    Q_PROPERTY(int currentThrottle READ currentThrottle NOTIFY currentThrottleChanged)
    Q_PROPERTY(int currentSpeedMode READ currentSpeedMode NOTIFY currentSpeedModeChanged)

public:
    explicit CommandEmitter(QObject *parent = nullptr);
    ~CommandEmitter();

public slots:
    void setTargetAddress(const QString& ip, quint16 port = 8888);
    void setSharedSocket(QUdpSocket* socket);
    void startEmitting(int intervalMs = 20); // 50 Hz = 20 ms
    void stopEmitting();

    int currentThrottle() const { return m_packet.throttleAxis; }
    int currentSpeedMode() const { return m_packet.speedModeLimit; }

signals:
    void currentThrottleChanged();
    void currentSpeedModeChanged();

public slots:   // Input hooks for the UI/Gamepad
    void updateThrottle(int16_t throttle);
    void updateSteering(int16_t steering);
    void setAutoBrake(bool enable);
    void setApfAvoidance(bool enable);
    void setRadarSweep(bool enable);
    void setRadarSweepSpeed(uint8_t speed);
    void setNoLagMode(bool enable);
    void setSpeedMode(uint8_t mode);
    void setHeadlightMode(int mode);
    void setCustomLedColor(int r, int g, int b);
    void setCustomLedPattern(uint8_t pattern);

private slots:
    void sendCommandPacket();

private:
    uint16_t calculateCrc16(const uint8_t *data, size_t length);

    QUdpSocket *m_socket;
    bool m_ownsSocket;
    QTimer *m_timer;
    QHostAddress m_targetIp;
    quint16 m_targetPort;
    
    VehicleCommandPacket m_packet;
};
