#include "CommandEmitter.hpp"
#include "NodeRegistry.hpp"
#include "TelemetryClient.hpp"
#include <QDateTime>
#include <QDebug>

CommandEmitter::CommandEmitter(NodeRegistry* registry, QObject* parent)
    : QObject(parent)
    , m_registry(registry)
    , m_socket(new QUdpSocket(this))
    , m_ownsSocket(true)
    , m_timer(new QTimer(this))
    , m_pingTimer(new QTimer(this))
{
    memset(&m_packet, 0, sizeof(VehicleCommandPacket));
    m_packet.preamble   = LBP_PREAMBLE_CMD;
    m_packet.sequenceId = 0;

    connect(m_timer,     &QTimer::timeout, this, &CommandEmitter::sendCommandPacket);
    connect(m_pingTimer, &QTimer::timeout, this, &CommandEmitter::sendPing);
    m_pingTimer->setInterval(1000); // 1 Hz ping
    m_pingTimer->start();
}

CommandEmitter::~CommandEmitter() {
    stopEmitting();
    if (m_ownsSocket && m_socket) {
        m_socket->deleteLater();
    }
}

void CommandEmitter::setSharedSocket(QUdpSocket* socket) {
    if (m_ownsSocket && m_socket) {
        m_socket->deleteLater();
    }
    m_socket = socket;
    m_ownsSocket = false;
}

void CommandEmitter::setTargetAddress(const QString& ip, quint16 port) {
    bool isNewIp = (m_targetIp != QHostAddress(ip));
    m_targetIp   = QHostAddress(ip);
    m_targetPort = port;
    if (isNewIp) {
        // New session — reset sequence counter so rover can detect restarts
        m_packet.sequenceId = 0;
        qDebug() << "[CommandEmitter] New target" << ip << "— sequence reset";
    }
}

void CommandEmitter::startEmitting(int intervalMs) {
    if (!m_timer->isActive()) {
        m_timer->start(intervalMs);
    }
}

void CommandEmitter::stopEmitting() {
    m_timer->stop();
    // Keep ping timer running — it maintains link awareness even when not commanding
}

void CommandEmitter::updateThrottle(int throttle) {
    if (m_packet.throttleAxis != throttle) {
        m_packet.throttleAxis = throttle;
        emit currentThrottleChanged();
    }
}
void CommandEmitter::updateSteering(int steering) {
    if (m_packet.steeringAxis != steering) {
        m_packet.steeringAxis = steering;
        emit currentSteeringChanged();
    }
}
void CommandEmitter::setAutoBrake(bool enable) { m_packet.enableAutoBrake = enable ? 1 : 0; }
void CommandEmitter::setApfAvoidance(bool enable) { m_packet.enableApfAvoidance = enable ? 1 : 0; }
void CommandEmitter::setRadarSweep(bool enable) { m_packet.enableRadarSweep = enable ? 1 : 0; }
void CommandEmitter::setRadarSweepSpeed(uint8_t speed) { m_packet.radarSweepSpeed = speed; }
void CommandEmitter::setNoLagMode(bool enable) { m_packet.enableNoLagMode = enable ? 1 : 0; }
void CommandEmitter::setSpeedMode(uint8_t mode) {
    if (m_packet.speedModeLimit != mode) {
        m_packet.speedModeLimit = mode;
        emit currentSpeedModeChanged();
    }
}
void CommandEmitter::setHeadlightMode(int mode) { m_packet.headlightMode = mode; }
void CommandEmitter::setCustomLedColor(int r, int g, int b) {
    m_packet.customLedR = r;
    m_packet.customLedG = g;
    m_packet.customLedB = b;
}

void CommandEmitter::setCustomLedPattern(uint8_t pattern) {
    m_packet.customLedPattern = pattern;
}


void CommandEmitter::setAutoTurn(bool enable, float targetHeading) {
    m_packet.enableAutoTurn = enable ? 1 : 0;
    m_packet.targetHeading = static_cast<int16_t>(targetHeading);
}

void CommandEmitter::sendCommandPacket() {
    if (m_targetIp.isNull()) return;

    m_packet.sequenceId++;

    size_t dataLen = sizeof(VehicleCommandPacket) - sizeof(uint16_t);
    m_packet.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&m_packet), dataLen);

    QByteArray datagram(reinterpret_cast<const char*>(&m_packet), sizeof(VehicleCommandPacket));
    qint64 sent = m_socket->writeDatagram(datagram, m_targetIp, m_targetPort);

    // Log once per second (every 50 packets at 50Hz)
    if (m_packet.sequenceId % 50 == 1) {
        qDebug() << "[CMD] seq:" << m_packet.sequenceId
                 << "to" << m_targetIp.toString() << ":" << m_targetPort
                 << "bytes:" << sent;
    }
}

void CommandEmitter::sendPing() {
    if (m_targetIp.isNull() || !m_socket) return;

    LbpPingPacket ping;
    ping.preamble  = LBP_PREAMBLE_PING;
    ping.seqId     = ++m_pingSeqId;
    ping.clientMs  = static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch() & 0xFFFFFFFF);

    size_t dataLen = sizeof(LbpPingPacket) - sizeof(uint16_t);
    ping.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&ping), dataLen);

    QByteArray datagram(reinterpret_cast<const char*>(&ping), sizeof(LbpPingPacket));
    m_socket->writeDatagram(datagram, m_targetIp, m_targetPort);

    // Tell TelemetryClient so it can match the PONG
    if (m_telemetryClient) {
        m_telemetryClient->recordPingSent(m_targetIp.toString(), ping.seqId,
                                          static_cast<qint64>(ping.clientMs));
    }
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
