#include "RoverNode.hpp"
#include <QDateTime>
#include <cstring>
#include <cmath>

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
    if (m_basePressurePa == 0.0f && tel.baroPressurePa > 50000.0f) {
        m_basePressurePa = tel.baroPressurePa; // Set initial baseline
    }

    if (m_hasTelemetry) {
        float alpha = 0.2f; // LPF factor

        // Linear variables (Baro & IMU)
        m_telemetry.pitchDeg = alpha * tel.pitchDeg + (1.0f - alpha) * m_telemetry.pitchDeg;
        m_telemetry.rollDeg = alpha * tel.rollDeg + (1.0f - alpha) * m_telemetry.rollDeg;
        m_telemetry.baroTempC = alpha * tel.baroTempC + (1.0f - alpha) * m_telemetry.baroTempC;
        m_telemetry.baroPressurePa = alpha * tel.baroPressurePa + (1.0f - alpha) * m_telemetry.baroPressurePa;
        m_telemetry.linearAccX = alpha * tel.linearAccX + (1.0f - alpha) * m_telemetry.linearAccX;
        m_telemetry.linearAccY = alpha * tel.linearAccY + (1.0f - alpha) * m_telemetry.linearAccY;
        m_telemetry.linearAccZ = alpha * tel.linearAccZ + (1.0f - alpha) * m_telemetry.linearAccZ;

        // Circular variables (Compass & Yaw)
        float diffHeading = tel.headingCompassDeg - m_telemetry.headingCompassDeg;
        while (diffHeading > 180.0f) diffHeading -= 360.0f;
        while (diffHeading < -180.0f) diffHeading += 360.0f;
        m_telemetry.headingCompassDeg += alpha * diffHeading;
        if (m_telemetry.headingCompassDeg < 0.0f) m_telemetry.headingCompassDeg += 360.0f;
        if (m_telemetry.headingCompassDeg >= 360.0f) m_telemetry.headingCompassDeg -= 360.0f;

        float diffYaw = tel.yawDeg - m_telemetry.yawDeg;
        while (diffYaw > 180.0f) diffYaw -= 360.0f;
        while (diffYaw < -180.0f) diffYaw += 360.0f;
        m_telemetry.yawDeg += alpha * diffYaw;
        if (m_telemetry.yawDeg < 0.0f) m_telemetry.yawDeg += 360.0f;
        if (m_telemetry.yawDeg >= 360.0f) m_telemetry.yawDeg -= 360.0f;

        // Copy unaffected fields
        m_telemetry.batteryVoltage = tel.batteryVoltage;
        m_telemetry.imuTempC = tel.imuTempC;
        m_telemetry.tof1DistMm = tel.tof1DistMm;
        m_telemetry.tof2DistMm = tel.tof2DistMm;
        m_telemetry.motorLeftPwm = tel.motorLeftPwm;
        m_telemetry.motorRightPwm = tel.motorRightPwm;
        m_telemetry.irArrayState = tel.irArrayState;
        m_telemetry.servoAngleDeg = tel.servoAngleDeg;
        m_telemetry.statusFlags = tel.statusFlags;
        m_telemetry.timestampMs = tel.timestampMs;
        m_telemetry.boardType = tel.boardType;
        m_telemetry.fwMajor = tel.fwMajor;
        m_telemetry.fwMinor = tel.fwMinor;
        m_telemetry.fwPatch = tel.fwPatch;
        m_telemetry.hardwareRev = tel.hardwareRev;
        m_telemetry.crc16 = tel.crc16;
    } else {
        m_telemetry = tel;
        m_hasTelemetry = true;
    }
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

float RoverNode::relativeAltitudeM() const {
    if (m_basePressurePa < 50000.0f || m_telemetry.baroPressurePa < 50000.0f) return 0.0f;
    return 44330.0f * (1.0f - std::pow(m_telemetry.baroPressurePa / m_basePressurePa, 1.0f / 5.255f));
}
