#include "CommandEmitter.hpp"

CommandEmitter::CommandEmitter(QObject *parent) 
    : QObject(parent), m_socket(new QUdpSocket(this)), m_timer(new QTimer(this)) {
    
    memset(&m_packet, 0, sizeof(VehicleCommandPacket));
    m_packet.preamble = 0x55AA;
    m_packet.sequenceId = 0;
    
    // Bind to 8889 with ShareAddress so outgoing packets originate from 8889.
    // This allows the ESP32's telemetry reply to 8889 to traverse the stateful firewall.
    m_socket->bind(QHostAddress::Any, 8889, QUdpSocket::ShareAddress);
    
    connect(m_timer, &QTimer::timeout, this, &CommandEmitter::sendCommandPacket);
}

CommandEmitter::~CommandEmitter() {
    stopEmitting();
}

void CommandEmitter::setTargetAddress(const QString& ip, quint16 port) {
    m_targetIp = QHostAddress(ip);
    m_targetPort = port;
}

void CommandEmitter::startEmitting(int intervalMs) {
    if (!m_timer->isActive()) {
        m_timer->start(intervalMs);
    }
}

void CommandEmitter::stopEmitting() {
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

void CommandEmitter::updateThrottle(int16_t throttle) { m_packet.throttleAxis = throttle; }
void CommandEmitter::updateSteering(int16_t steering) { m_packet.steeringAxis = steering; }
void CommandEmitter::setAutoBrake(bool enable) { m_packet.enableAutoBrake = enable ? 1 : 0; }
void CommandEmitter::setApfAvoidance(bool enable) { m_packet.enableApfAvoidance = enable ? 1 : 0; }
void CommandEmitter::setRadarSweep(bool enable) { m_packet.enableRadarSweep = enable ? 1 : 0; }
void CommandEmitter::setSpeedMode(uint8_t mode) { m_packet.speedModeLimit = mode; }

void CommandEmitter::sendCommandPacket() {
    if (m_targetIp.isNull()) return;

    m_packet.sequenceId++;
    
    size_t dataLen = sizeof(VehicleCommandPacket) - sizeof(uint16_t);
    m_packet.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&m_packet), dataLen);

    QByteArray datagram(reinterpret_cast<const char*>(&m_packet), sizeof(VehicleCommandPacket));
    m_socket->writeDatagram(datagram, m_targetIp, m_targetPort);
}

uint16_t CommandEmitter::calculateCrc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
