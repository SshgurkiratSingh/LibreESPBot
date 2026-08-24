#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include "../core/Types.hpp"

class CommandEmitter : public QObject {
    Q_OBJECT

public:
    explicit CommandEmitter(QObject *parent = nullptr);
    ~CommandEmitter();

    void setTargetAddress(const QString& ip, quint16 port = 8888);
    void startEmitting(int intervalMs = 20); // 50 Hz = 20 ms
    void stopEmitting();

    // Input hooks for the UI/Gamepad
    void updateThrottle(int16_t throttle);
    void updateSteering(int16_t steering);
    void setAutoBrake(bool enable);
    void setApfAvoidance(bool enable);
    void setRadarSweep(bool enable);
    void setSpeedMode(uint8_t mode);

private slots:
    void sendCommandPacket();

private:
    uint16_t calculateCrc16(const uint8_t *data, size_t length);

    QUdpSocket *m_socket;
    QTimer *m_timer;
    QHostAddress m_targetIp;
    quint16 m_targetPort;
    
    VehicleCommandPacket m_packet;
};
