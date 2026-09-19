#include "DiscoveryWorker.hpp"
#include "NodeRegistry.hpp"
#include "RoverNode.hpp"
#include <QNetworkDatagram>
#include <QDebug>

DiscoveryWorker::DiscoveryWorker(NodeRegistry* registry, QObject* parent)
    : QObject(parent)
    , m_registry(registry)
    , m_socket(new QUdpSocket(this))
    , m_mdnsSocket(new QUdpSocket(this))
{
}

DiscoveryWorker::~DiscoveryWorker() {
    m_socket->close();
    m_mdnsSocket->close();
}

void DiscoveryWorker::startDiscovery() {
    // Bind to LBP_PORT_BEACON (4210) — dedicated, no OS mDNS interference.
    // ShareAddress allows multiple sockets on the same port (useful on Android).
    if (m_socket->bind(QHostAddress::AnyIPv4, LBP_PORT_BEACON,
                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        connect(m_socket, &QUdpSocket::readyRead, this, &DiscoveryWorker::readPendingDatagrams);
        qDebug() << "[DiscoveryWorker] Listening for LBP2 beacons on port" << LBP_PORT_BEACON;
    } else {
        qWarning() << "[DiscoveryWorker] Failed to bind to port" << LBP_PORT_BEACON;
    }

    // Bind mDNS socket for camera discovery
    if (m_mdnsSocket->bind(QHostAddress::AnyIPv4, 5353,
                           QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        m_mdnsSocket->joinMulticastGroup(QHostAddress("224.0.0.251"));
        connect(m_mdnsSocket, &QUdpSocket::readyRead, this, &DiscoveryWorker::readPendingMdnsDatagrams);
        qDebug() << "[DiscoveryWorker] Listening for camera mDNS beacons on port 5353";
    } else {
        qWarning() << "[DiscoveryWorker] Failed to bind mDNS socket to port 5353";
    }
}

void DiscoveryWorker::readPendingDatagrams() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();

        // Need at least 2 bytes for preamble check
        if (data.size() < 2) continue;

        const uint16_t preamble = *reinterpret_cast<const uint16_t*>(data.constData());
        if (preamble != LBP_PREAMBLE_BCN) continue;
        if (data.size() != static_cast<int>(sizeof(LbpBeaconPacket))) {
            qWarning() << "[DiscoveryWorker] Beacon size mismatch: expected"
                       << sizeof(LbpBeaconPacket) << "got" << data.size();
            continue;
        }

        LbpBeaconPacket bcn;
        memcpy(&bcn, data.constData(), sizeof(bcn));

        // Validate CRC
        size_t dataLen = sizeof(LbpBeaconPacket) - sizeof(uint16_t);
        if (calculateCrc16(reinterpret_cast<const uint8_t*>(&bcn), dataLen) != bcn.crc16) {
            qWarning() << "[DiscoveryWorker] Beacon CRC mismatch";
            continue;
        }

        QString senderIp = datagram.senderAddress().toString();
        if (senderIp.startsWith(QStringLiteral("::ffff:")))
            senderIp = senderIp.mid(7);

        // Null-terminate boardName for safety before making a QString
        char safeName[17];
        memcpy(safeName, bcn.boardName, 16);
        safeName[16] = '\0';

        qDebug() << "[DiscoveryWorker] Beacon from" << senderIp
                 << "board:" << safeName
                 << "fw:" << bcn.fwMajor << "." << bcn.fwMinor << "." << bcn.fwPatch
                 << "uptime:" << bcn.uptimeMs << "ms";

        // Route to NodeRegistry — creates node if new, updates if existing
        RoverNode* node = m_registry->nodeForIp(senderIp);
        node->updateFromBeacon(bcn, senderIp);
    }
}

void DiscoveryWorker::readPendingMdnsDatagrams() {
    while (m_mdnsSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_mdnsSocket->receiveDatagram();
        QByteArray data = datagram.data();
        QString msg = QString::fromUtf8(data);

        // Check if it matches the camera broadcast format:
        // "_camctrl._udp.local drv=ESP32-CAM ip=x.x.x.x"
        if (msg.startsWith("_camctrl._udp.local") && msg.contains("ip=")) {
            int ipIndex = msg.indexOf("ip=") + 3;
            QString ip = msg.mid(ipIndex).trimmed();
            
            if (m_cameraIp != ip) {
                m_cameraIp = ip;
                emit cameraDiscovered(ip);
                qDebug() << "[DiscoveryWorker] Auto-discovered camera IP:" << ip;
            }
        }
    }
}

void DiscoveryWorker::setManualIp(const QString& ip) {
    qDebug() << "[DiscoveryWorker] Manual rover IP:" << ip;
    m_registry->addManualNode(ip);
}

void DiscoveryWorker::setManualCameraIp(const QString& ip) {
    if (m_cameraIp != ip) {
        m_cameraIp = ip;
        emit cameraDiscovered(ip);
        qDebug() << "[DiscoveryWorker] Manual camera IP:" << ip;
    }
}

uint16_t DiscoveryWorker::calculateCrc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int j = 0; j < 8; ++j)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}
