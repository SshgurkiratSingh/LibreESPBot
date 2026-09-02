#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include "../core/Types.hpp"

class TelemetryClient : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(float pitch READ pitch NOTIFY telemetryUpdated)
    Q_PROPERTY(float roll READ roll NOTIFY telemetryUpdated)
    Q_PROPERTY(float yaw READ yaw NOTIFY telemetryUpdated)
    Q_PROPERTY(float headingCompassDeg READ headingCompassDeg NOTIFY telemetryUpdated)
    Q_PROPERTY(float batteryVoltage READ batteryVoltage NOTIFY telemetryUpdated)
    Q_PROPERTY(float imuTempC READ imuTempC NOTIFY telemetryUpdated)
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

public:
    explicit TelemetryClient(QObject *parent = nullptr);
    ~TelemetryClient();

    void startListening(quint16 port = 8889);
    QUdpSocket* socket() const { return m_socket; }
    
    float pitch() const { return m_packet.pitchDeg; }
    float roll() const { return m_packet.rollDeg; }
    float yaw() const { return m_packet.yawDeg; }
    float headingCompassDeg() const { return m_packet.headingCompassDeg; }
    float batteryVoltage() const { return m_packet.batteryVoltage; }
    float imuTempC() const { return m_packet.imuTempC; }
    int tof1DistMm() const { return m_packet.tof1DistMm; }
    int tof2DistMm() const { return m_packet.tof2DistMm; }
    int servoAngleDeg() const { return m_packet.servoAngleDeg; }
    float linearAccX() const { return m_packet.linearAccX; }
    float linearAccY() const { return m_packet.linearAccY; }
    float linearAccZ() const { return m_packet.linearAccZ; }
    int motorLeftPwm() const { return m_packet.motorLeftPwm; }
    int motorRightPwm() const { return m_packet.motorRightPwm; }
    int activeImuType() const { return m_packet.activeImuType; }
    int activeMagType() const { return m_packet.activeMagType; }
    int statusFlags() const { return m_packet.statusFlags; }

signals:
    void telemetryUpdated();
    void connectionLost();

private slots:
    void readPendingDatagrams();

private:
    uint16_t calculateCrc16(const uint8_t *data, size_t length);

    QUdpSocket *m_socket;
    VehicleTelemetryPacket m_packet;
    qint64 m_lastPacketTime;
};
