#include "DiscoveryWorker.hpp"
#include <QNetworkDatagram>
#include <QDebug>

DiscoveryWorker::DiscoveryWorker(QObject *parent) 
    : QObject(parent), m_socket(new QUdpSocket(this)) {
}

DiscoveryWorker::~DiscoveryWorker() {
    m_socket->close();
}

void DiscoveryWorker::startDiscovery() {
    // Listen for mDNS traffic on multicast group 224.0.0.251 port 5353
    if (m_socket->bind(QHostAddress::AnyIPv4, 5353, QUdpSocket::ShareAddress)) {
        m_socket->joinMulticastGroup(QHostAddress("224.0.0.251"));
        connect(m_socket, &QUdpSocket::readyRead, this, &DiscoveryWorker::readPendingDatagrams);
        qDebug() << "DiscoveryWorker started mDNS listening.";
    } else {
        qWarning() << "DiscoveryWorker failed to bind to mDNS port.";
    }
}

void DiscoveryWorker::setManualIp(const QString& ip) {
    if (m_roverIp != ip) {
        m_roverIp = ip;
        m_hardwareProfile = "Manual Override";
        emit roverDiscovered(m_roverIp, m_hardwareProfile);
        qDebug() << "Manual IP set to:" << m_roverIp;
    }
}

void DiscoveryWorker::readPendingDatagrams() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();
        
        // Very basic mock mDNS parser looking for our specific service string
        // In a real Qt app, QZeroConf or QDnsLookup is preferred, but for raw UDP:
        if (data.contains("_roverctrl._udp.local")) {
            QString senderIp = datagram.senderAddress().toString();
            if (senderIp.startsWith("::ffff:")) {
                senderIp = senderIp.mid(7);
            }
            
            // Extract TXT records manually (mock logic)
            // Example TXT structure embedded in UDP packet payload
            int drvIdx = data.indexOf("drv=");
            if (drvIdx != -1) {
                if (m_roverIp != senderIp) {
                    m_roverIp = senderIp;
                    m_hardwareProfile = "Rover V2"; // Extract actual string in prod
                    emit roverDiscovered(m_roverIp, m_hardwareProfile);
                    qDebug() << "Discovered Rover at:" << m_roverIp;
                }
            }
        }
    }
}
