#include "TelemetryClient.hpp"
#include "RoverNode.hpp"
#include <QDateTime>
#include <QNetworkDatagram>
#include <QDebug>

TelemetryClient::TelemetryClient(NodeRegistry* registry, QObject* parent)
    : QObject(parent)
    , m_registry(registry)
    , m_socket(new QUdpSocket(this))
{
    // Health watchdog: checks every 500 ms whether the active node is still alive.
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
        qDebug() << "[TelemetryClient] Listening on port" << port;
    } else {
        qCritical() << "[TelemetryClient] Failed to bind to port" << port;
    }
}

void TelemetryClient::readPendingDatagrams() {
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        QByteArray data = datagram.data();

        QString senderIp = datagram.senderAddress().toString();
        if (senderIp.startsWith(QStringLiteral("::ffff:")))
            senderIp = senderIp.mid(7);

        if (data.size() < 2) continue;
        const uint16_t preamble = *reinterpret_cast<const uint16_t*>(data.constData());

        if (preamble == LBP_PREAMBLE_TEL && data.size() == static_cast<int>(sizeof(VehicleTelemetryPacket))) {
            VehicleTelemetryPacket pkt;
            memcpy(&pkt, data.constData(), sizeof(pkt));

            size_t dataLen = sizeof(VehicleTelemetryPacket) - sizeof(uint16_t);
            if (calculateCrc16(reinterpret_cast<const uint8_t*>(&pkt), dataLen) != pkt.crc16) {
                qWarning() << "[TelemetryClient] CRC mismatch from" << senderIp;
                continue;
            }

            RoverNode* node = m_registry->nodeForIp(senderIp);
            node->updateFromTelemetry(pkt);
            emit telemetryUpdated();

        } else if (preamble == LBP_PREAMBLE_PONG && data.size() == static_cast<int>(sizeof(LbpPongPacket))) {
            LbpPongPacket pong;
            memcpy(&pong, data.constData(), sizeof(pong));

            size_t dataLen = sizeof(LbpPongPacket) - sizeof(uint16_t);
            if (calculateCrc16(reinterpret_cast<const uint8_t*>(&pong), dataLen) != pong.crc16) {
                qWarning() << "[TelemetryClient] PONG CRC mismatch from" << senderIp;
                continue;
            }

            auto it = m_pendingPings.find(pong.seqId);
            if (it != m_pendingPings.end() && it->ip == senderIp) {
                qint64 rtt = QDateTime::currentMSecsSinceEpoch() - it->sentMs;
                RoverNode* node = m_registry->nodeForIp(senderIp);
                node->updateFromPong(pong, it->sentMs);
                qDebug() << "[TelemetryClient] PONG from" << senderIp << "RTT:" << rtt << "ms";
                m_pendingPings.erase(it);
            }
        } else {
            qDebug() << "[TelemetryClient] Unknown packet from" << senderIp
                     << "preamble:" << Qt::hex << preamble << "size:" << data.size();
        }
    }
}

void TelemetryClient::checkConnectionHealth() {
    // Delegate health checking to each node via NodeRegistry
    // (NodeRegistry's own healthTimer calls checkHealth() on each node)
    bool nowConnected = connected();
    if (nowConnected != m_wasConnected) {
        m_wasConnected = nowConnected;
        emit connectionStateChanged();
        if (!nowConnected) {
            emit connectionLost();
            qWarning() << "[TelemetryClient] Active node connection lost";
        }
    }
}

void TelemetryClient::recordPingSent(const QString& ip, uint16_t seqId, qint64 timestampMs) {
    m_pendingPings[seqId] = PingRecord{ip, timestampMs};
    // Expire stale pings > 10s old
    for (auto it = m_pendingPings.begin(); it != m_pendingPings.end(); ) {
        if (timestampMs - it->sentMs > 10000) it = m_pendingPings.erase(it);
        else ++it;
    }
}

// ── Property delegators ───────────────────────────────────────────────────────
static const VehicleTelemetryPacket s_emptyTel{};

const VehicleTelemetryPacket& activeTel(NodeRegistry* reg) {
    RoverNode* n = reg->activeNode();
    return n ? n->telemetry() : s_emptyTel;
}

bool  TelemetryClient::connected()         const { auto* n=m_registry->activeNode(); return n && n->connected(); }
float TelemetryClient::pitch()             const { return activeTel(m_registry).pitchDeg; }
float TelemetryClient::roll()              const { return activeTel(m_registry).rollDeg; }
float TelemetryClient::yaw()               const { return activeTel(m_registry).yawDeg; }
float TelemetryClient::headingCompassDeg() const { return activeTel(m_registry).headingCompassDeg; }
float TelemetryClient::batteryVoltage()    const { return activeTel(m_registry).batteryVoltage; }
float TelemetryClient::imuTempC()          const { return activeTel(m_registry).imuTempC; }
float TelemetryClient::baroTempC()         const { return activeTel(m_registry).baroTempC; }
float TelemetryClient::baroPressurePa()    const { return activeTel(m_registry).baroPressurePa; }
int   TelemetryClient::irArrayState()      const { return activeTel(m_registry).irArrayState; }
int   TelemetryClient::tof1DistMm()        const { return activeTel(m_registry).tof1DistMm; }
int   TelemetryClient::tof2DistMm()        const { return activeTel(m_registry).tof2DistMm; }
int   TelemetryClient::servoAngleDeg()     const { return activeTel(m_registry).servoAngleDeg; }
float TelemetryClient::linearAccX()        const { return activeTel(m_registry).linearAccX; }
float TelemetryClient::linearAccY()        const { return activeTel(m_registry).linearAccY; }
float TelemetryClient::linearAccZ()        const { return activeTel(m_registry).linearAccZ; }
int   TelemetryClient::motorLeftPwm()      const { return activeTel(m_registry).motorLeftPwm; }
int   TelemetryClient::motorRightPwm()     const { return activeTel(m_registry).motorRightPwm; }
int   TelemetryClient::activeImuType()     const { return activeTel(m_registry).activeImuType; }
int   TelemetryClient::activeMagType()     const { return activeTel(m_registry).activeMagType; }
int   TelemetryClient::statusFlags()       const { return activeTel(m_registry).statusFlags; }

QString TelemetryClient::firmwareVersion() const {
    auto* n = m_registry->activeNode(); return n ? n->fwVersion() : QStringLiteral("?.?.?");
}
QString TelemetryClient::boardName() const {
    auto* n = m_registry->activeNode(); return n ? n->boardName() : QStringLiteral("Unknown");
}
bool TelemetryClient::versionMismatch() const {
    auto* n = m_registry->activeNode(); return n && n->versionMismatch();
}

uint16_t TelemetryClient::calculateCrc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int j = 0; j < 8; ++j)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}
