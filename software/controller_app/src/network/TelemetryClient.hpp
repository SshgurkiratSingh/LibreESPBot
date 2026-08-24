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
    Q_PROPERTY(float batteryVoltage READ batteryVoltage NOTIFY telemetryUpdated)
    Q_PROPERTY(int tof1DistMm READ tof1DistMm NOTIFY telemetryUpdated)
    Q_PROPERTY(int tof2DistMm READ tof2DistMm NOTIFY telemetryUpdated)
    Q_PROPERTY(int servoAngleDeg READ servoAngleDeg NOTIFY telemetryUpdated)

public:
    explicit TelemetryClient(QObject *parent = nullptr);
    ~TelemetryClient();

    void startListening(quint16 port = 8889);
    
    float pitch() const { return m_packet.pitchDeg; }
    float roll() const { return m_packet.rollDeg; }
    float yaw() const { return m_packet.yawDeg; }
    float batteryVoltage() const { return m_packet.batteryVoltage; }
    int tof1DistMm() const { return m_packet.tof1DistMm; }
    int tof2DistMm() const { return m_packet.tof2DistMm; }
    int servoAngleDeg() const { return m_packet.servoAngleDeg; }

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
