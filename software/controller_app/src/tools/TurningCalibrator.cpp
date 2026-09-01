#include "TurningCalibrator.hpp"
#include <QDebug>
#include <cmath>

TurningCalibrator::TurningCalibrator(CommandEmitter* emitter, TelemetryClient* telemetry, QObject *parent)
    : QObject(parent)
    , m_emitter(emitter)
    , m_telemetry(telemetry)
    , m_isRunning(false)
    , m_currentTestThrottle(200)
    , m_optimalThrottle(0)
    , m_initialYaw(0)
{
    m_testTimer.setSingleShot(true);
    connect(&m_testTimer, &QTimer::timeout, this, &TurningCalibrator::stopAndEvaluate);
}

void TurningCalibrator::startCalibration() {
    if (m_isRunning || !m_emitter || !m_telemetry) return;

    m_currentTestThrottle = 200;
    m_optimalThrottle = 0;
    setRunning(true);
    
    runNextTest();
}

void TurningCalibrator::cancelCalibration() {
    if (!m_isRunning) return;

    m_testTimer.stop();
    if (m_emitter) {
        m_emitter->updateSteering(0);
        m_emitter->updateThrottle(0);
    }

    setRunning(false);
    setStatus("Calibration cancelled.");
}

void TurningCalibrator::runNextTest() {
    if (!m_isRunning) return;
    
    if (m_currentTestThrottle > 1000) {
        setStatus("Failed to find optimal throttle.");
        cancelCalibration();
        return;
    }

    m_initialYaw = m_telemetry->yaw();
    setStatus(QString("Testing Throttle: %1...").arg(m_currentTestThrottle));

    // Command a full right turn at test throttle
    m_emitter->updateSteering(512); // Right turn
    m_emitter->updateThrottle(m_currentTestThrottle);

    // Run for 800ms
    m_testTimer.start(800);
}

void TurningCalibrator::stopAndEvaluate() {
    if (!m_isRunning) return;

    // Stop moving
    m_emitter->updateSteering(0);
    m_emitter->updateThrottle(0);

    // Wait a brief moment to stabilize and read final yaw
    // (In a real system, you might use another timer here. For simplicity, we'll assume telemetry updates are fast enough).
    float currentYaw = m_telemetry->yaw();
    float diff = qAbs(angleDifference(m_initialYaw, currentYaw));

    qDebug() << "Test Throttle:" << m_currentTestThrottle << "Yielded rotation:" << diff << "degrees";

    // If it rotated at least 15 degrees in 800ms, it's a good turning throttle!
    if (diff >= 15.0f) {
        m_optimalThrottle = m_currentTestThrottle;
        emit optimalThrottleChanged();
        
        setStatus(QString("Success! Optimal turn throttle found: %1").arg(m_optimalThrottle));
        emit calibrationFinished(m_optimalThrottle);
        
        setRunning(false);
    } else {
        // Not enough torque to turn. Increase throttle and try again.
        m_currentTestThrottle += 50;
        emit currentTestThrottleChanged();
        
        // Wait 500ms before starting next test
        QTimer::singleShot(500, this, [this]() {
            if (m_isRunning) runNextTest();
        });
    }
}

float TurningCalibrator::angleDifference(float target, float current) {
    float diff = target - current;
    while (diff < -180.0f) diff += 360.0f;
    while (diff > 180.0f) diff -= 360.0f;
    return diff;
}

void TurningCalibrator::setRunning(bool r) {
    if (m_isRunning != r) {
        m_isRunning = r;
        emit isRunningChanged();
    }
}

void TurningCalibrator::setStatus(const QString& msg) {
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged();
    }
}
