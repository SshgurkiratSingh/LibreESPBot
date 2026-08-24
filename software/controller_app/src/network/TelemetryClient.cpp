#include "TelemetryClient.hpp"
#include <QDateTime>
#include <QDebug>

TelemetryClient::TelemetryClient(QObject *parent) 
    : QObject(parent), m_socket(new QUdpSocket(this)), m_lastPacketTime(0) {
    memset(&m_packet, 0, sizeof(VehicleTelemetryPacket));
}

TelemetryClient::~TelemetryClient() {
    m_socket->close();
}

void TelemetryClient::startListening(quint16 port) {
    if (m_socket->bind(QHostAddress::Any, port)) {
        connect(m_socket, &QUdpSocket::readyRead, this, &TelemetryClient::readPendingDatagrams);
        qDebug() << "TelemetryClient listening on port:" << port;
    } else {
        qCritical() << "Failed to bind TelemetryClient to port:" << port;
    }
}

void TelemetryClient::readPendingDatagrams() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();
        
        if (data.size() == sizeof(VehicleTelemetryPacket)) {
            VehicleTelemetryPacket packet;
            memcpy(&packet, data.constData(), sizeof(VehicleTelemetryPacket));
            
            // Check Preamble
            if (packet.preamble != 0xAA55) continue;
            
            // CRC-16-CCITT Verification
            size_t dataLen = sizeof(VehicleTelemetryPacket) - sizeof(uint16_t);
            uint16_t computedCrc = calculateCrc16(reinterpret_cast<const uint8_t*>(&packet), dataLen);
            
            if (computedCrc == packet.crc16) {
                m_packet = packet;
                m_lastPacketTime = QDateTime::currentMSecsSinceEpoch();
                emit telemetryUpdated();
            } else {
                qWarning() << "Telemetry CRC mismatch. Computed:" << computedCrc << "Received:" << packet.crc16;
            }
        }
    }
}

// CRC-16-CCITT algorithm implementation
uint16_t TelemetryClient::calculateCrc16(const uint8_t *data, size_t length) {
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
