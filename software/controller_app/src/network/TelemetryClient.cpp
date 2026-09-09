#include "TelemetryClient.hpp"
#include <QDateTime>
#include <QDebug>

TelemetryClient::TelemetryClient(QObject *parent) 
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_lastPacketTime(0)
    , m_connected(false)
    , m_logRateLimit(0)
{
    memset(&m_packet, 0, sizeof(VehicleTelemetryPacket));

    // Watchdog: check every 500ms whether we've heard from the rover recently.
    // If 2500ms pass with no valid packet → declare connection lost.
    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setInterval(500);
    connect(m_watchdogTimer, &QTimer::timeout, this, &TelemetryClient::checkConnectionHealth);
    m_watchdogTimer->start();
}

TelemetryClient::~TelemetryClient() {
    m_socket->close();
}

void TelemetryClient::startListening(quint16 port) {
    if (m_socket->bind(QHostAddress::Any, port, QUdpSocket::ShareAddress)) {
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
            if (packet.preamble != 0xAA55) {
                qWarning() << "[Telemetry] Invalid preamble:" << Qt::hex << packet.preamble;
                continue;
            }
            
            // CRC-16-CCITT Verification
            size_t dataLen = sizeof(VehicleTelemetryPacket) - sizeof(uint16_t);
            uint16_t computedCrc = calculateCrc16(reinterpret_cast<const uint8_t*>(&packet), dataLen);
            
            if (computedCrc == packet.crc16) {
                qint64 now = QDateTime::currentMSecsSinceEpoch();

                // Mark connection as established
                if (!m_connected) {
                    m_connected = true;
                    emit connectionStateChanged();
                }
                m_packet = packet;
                m_lastPacketTime = now;
                
                QString ip = datagram.senderAddress().toString();
                if (ip.startsWith("::ffff:")) ip = ip.mid(7); // Normalize IPv4-mapped IPv6
                if (!m_discoveredIps.contains(ip)) {
                    m_discoveredIps.append(ip);
                    emit discoveredIpsChanged();
                    emit botDiscovered(ip);
                }
                
                emit telemetryUpdated();

                // Rate-limited debug log: print at most once per second to avoid UI thread spam
                if (now - m_logRateLimit > 1000) {
                    m_logRateLimit = now;
                    qDebug() << "[Telemetry] OK — seq:" << packet.timestampMs
                             << "| size:" << data.size()
                             << "| battery:" << packet.batteryVoltage << "V";
                }
            } else {
                qWarning() << "[Telemetry] CRC mismatch. Computed:" << Qt::hex << computedCrc
                           << "Received:" << packet.crc16;
            }
        } else {
            qWarning() << "[Telemetry] Dropped packet! Size mismatch. Expected:"
                       << sizeof(VehicleTelemetryPacket) << "Got:" << data.size();
        }
    }
}

void TelemetryClient::checkConnectionHealth() {
    if (m_lastPacketTime == 0) return; // Never received any packet yet — not connected
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 silence = now - m_lastPacketTime;

    // 2500ms silence = rover disconnected (the rover sends at 20Hz, so 50ms per packet;
    // 2500ms = 50 missed packets — definitely not just a hiccup)
    if (silence > 2500 && m_connected) {
        m_connected = false;
        emit connectionStateChanged();
        emit connectionLost();
        qWarning() << "[Telemetry] Connection lost! No packet for" << silence << "ms";
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
