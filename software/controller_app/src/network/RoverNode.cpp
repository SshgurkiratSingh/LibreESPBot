#include "RoverNode.hpp"
#include <QDateTime>
#include <cstring>

RoverNode::RoverNode(const QString& ip, QObject* parent)
    : QObject(parent), m_ip(ip) {
    memset(&m_telemetry, 0, sizeof(m_telemetry));
}

void RoverNode::updateFromBeacon(const LbpBeaconPacket& bcn, const QString& senderIp) {
    Q_UNUSED(senderIp);
    // Ensure null termination for boardName up to 16 chars
    m_boardName = QString::fromLocal8Bit(bcn.boardName, strnlen(bcn.boardName, 16));
    m_fwVersion = QString("%1.%2.%3").arg(bcn.fwMajor).arg(bcn.fwMinor).arg(bcn.fwPatch);
    m_boardType = bcn.boardType;
    m_versionMismatch = (bcn.telemetrySize != sizeof(VehicleTelemetryPacket) || 
                         bcn.commandSize != sizeof(VehicleCommandPacket));

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_lastSeenMs = now;
    m_lastBeaconMs = now;

    emit updated();
}

void RoverNode::updateFromTelemetry(const VehicleTelemetryPacket& tel) {
    m_telemetry = tel;
    m_hasTelemetry = true;
    m_battery = tel.batteryVoltage;
    m_boardType = tel.boardType;
    m_fwVersion = QString("%1.%2.%3").arg(tel.fwMajor).arg(tel.fwMinor).arg(tel.fwPatch);
    
    switch (m_boardType) {
        case BOARD_S3_STD:   m_boardName = "ESP32-S3 STD"; break;
        case BOARD_ROVER_V2: m_boardName = "Rover V2"; break;
        default:             m_boardName = "Unknown"; break;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_lastTelemetryMs = now;
    m_lastSeenMs = now;

    emit updated();
}

void RoverNode::updateFromPong(const LbpPongPacket& pong, qint64 pingTimeSentMs) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_rttMs = static_cast<int>(now - pingTimeSentMs);
    m_battery = pong.batteryVoltage;
    m_lastSeenMs = now;

    emit updated();
}

void RoverNode::checkHealth() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool newlyConnected = (now - m_lastTelemetryMs) < CONNECTED_TIMEOUT_MS && m_hasTelemetry;
    
    if (m_connected != newlyConnected) {
        m_connected = newlyConnected;
        emit updated();
    }
}

QString RoverNode::statusText() const {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_hasTelemetry && m_connected) {
        return "CONNECTED";
    } else if (m_lastBeaconMs > 0 && (now - m_lastSeenMs) < STALE_TIMEOUT_MS) {
        return "BEACON ONLY";
    } else if (m_hasTelemetry && !m_connected) {
        return "DISCONNECTED";
    } else {
        return "STALE";
    }
}
