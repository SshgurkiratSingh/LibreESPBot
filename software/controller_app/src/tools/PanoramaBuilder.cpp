#include "PanoramaBuilder.hpp"
#include <QPainter>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <cmath>

PanoramaBuilder::PanoramaBuilder(CommandEmitter* emitter, TelemetryClient* telemetry, VideoManager* video, QObject *parent)
    : QObject(parent)
    , m_emitter(emitter)
    , m_telemetry(telemetry)
    , m_video(video)
    , m_state(IDLE)
    , m_isRunning(false)
    , m_progressPercent(0)
    , m_stepDegrees(30)
    , m_targetTotalShots(12) // 360 / 30
    , m_turnThrottle(400)
    , m_initialYaw(0)
    , m_targetYaw(0)
    , m_totalTurned(0)
{
    m_stabilizeTimer.setSingleShot(true);
    connect(&m_stabilizeTimer, &QTimer::timeout, this, &PanoramaBuilder::stabilizeAndCapture);

    if (m_telemetry) {
        connect(m_telemetry, &TelemetryClient::telemetryUpdated, this, &PanoramaBuilder::onTelemetryUpdated);
    }
}

void PanoramaBuilder::setStepDegrees(int deg) {
    if (m_stepDegrees != deg && deg > 0) {
        m_stepDegrees = deg;
        emit stepDegreesChanged();
    }
}

void PanoramaBuilder::setTurnThrottle(int t) {
    if (m_turnThrottle != t) {
        m_turnThrottle = t;
        emit turnThrottleChanged();
    }
}

void PanoramaBuilder::startPanorama() {
    if (m_isRunning || !m_emitter || !m_telemetry || !m_video) return;

    m_capturedImages.clear();
    emit totalCapturedChanged();

    m_targetTotalShots = 360 / qMax(5, m_stepDegrees);
    m_initialYaw = m_telemetry->yaw();
    m_totalTurned = 0;
    
    setRunning(true);
    setProgress(0);
    setLastResultPath("");

    // Capture first frame immediately
    m_state = STABILIZING;
    m_stabilizeTimer.start(500);
}

void PanoramaBuilder::cancelPanorama() {
    if (!m_isRunning) return;

    m_stabilizeTimer.stop();
    if (m_emitter) {
        m_emitter->updateSteering(0);
    }

    m_state = IDLE;
    setRunning(false);
    setProgress(0);
    emit panoramaError("Panorama cancelled by user.");
}

void PanoramaBuilder::executeNextTurn() {
    if (!m_isRunning) return;

    if (m_capturedImages.size() >= m_targetTotalShots) {
        // We have a full 360 view
        stitchImages();
        return;
    }

    // Set new target yaw (we want to turn 'stepDegrees' right)
    m_targetYaw = normalizeAngle(m_telemetry->yaw() + m_stepDegrees);
    
    m_state = ROTATING;
    
    // Command the bot to rotate in place (pivot right)
    // Positive steering is right. We need throttle for the bot to actually move and turn.
    m_emitter->updateSteering(512); // 50% of 1023
    m_emitter->updateThrottle(m_turnThrottle);
}

float PanoramaBuilder::normalizeAngle(float angle) {
    while (angle < 0) angle += 360;
    while (angle >= 360) angle -= 360;
    return angle;
}

float PanoramaBuilder::angleDifference(float target, float current) {
    float diff = target - current;
    while (diff < -180.0f) diff += 360.0f;
    while (diff > 180.0f) diff -= 360.0f;
    return diff;
}

void PanoramaBuilder::onTelemetryUpdated() {
    if (m_state != ROTATING || !m_isRunning) return;

    float currentYaw = m_telemetry->yaw();
    
    // We are rotating right, so we want the diff to cross 0
    float diff = angleDifference(m_targetYaw, currentYaw);

    // If we're within 5 degrees of target, stop.
    // Or if diff becomes negative (we overshot, but within a reasonable margin to avoid noise triggers)
    if (qAbs(diff) <= 5.0f || (diff < 0.0f && diff > -45.0f)) {
        // Stop turning
        m_emitter->updateSteering(0);
        m_state = STABILIZING;
        
        // Wait 1 second for chassis to settle before taking picture
        m_stabilizeTimer.start(1000);
    }
}

void PanoramaBuilder::stabilizeAndCapture() {
    if (!m_isRunning || !m_video) return;

    QString b64 = m_video->currentFrameBase64();
    if (!b64.isEmpty()) {
        if (b64.startsWith("data:image/jpeg;base64,")) {
            b64 = b64.mid(23);
        }
        QByteArray ba = QByteArray::fromBase64(b64.toUtf8());
        QImage img;
        if (img.loadFromData(ba)) {
            m_capturedImages.append(img);
            emit totalCapturedChanged();
            
            setProgress((m_capturedImages.size() * 100) / m_targetTotalShots);
        } else {
            qWarning() << "Failed to decode image from base64 data.";
        }
    } else {
        qWarning() << "Failed to capture frame for panorama";
        cancelPanorama();
        emit panoramaError("Failed to capture video frame.");
        return;
    }

    executeNextTurn();
}

void PanoramaBuilder::stitchImages() {
    m_state = STITCHING;
    setProgress(95);

    if (m_capturedImages.isEmpty()) {
        cancelPanorama();
        return;
    }

    // Naive horizontal concatenation
    int singleWidth = m_capturedImages.first().width();
    int height = m_capturedImages.first().height();
    int totalWidth = singleWidth * m_capturedImages.size();

    QImage result(totalWidth, height, QImage::Format_ARGB32);
    result.fill(Qt::black);

    QPainter painter(&result);
    for (int i = 0; i < m_capturedImages.size(); ++i) {
        painter.drawImage(i * singleWidth, 0, m_capturedImages[i]);
    }
    painter.end();

    QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir dir(docsPath);
    if (!dir.exists("LibreESP")) {
        dir.mkpath("LibreESP");
    }
    
    QString fileName = QString("Panorama_%1.jpg").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString fullPath = dir.filePath("LibreESP/" + fileName);

    if (result.save(fullPath, "JPG", 90)) {
        setLastResultPath(fullPath);
        emit panoramaFinished(fullPath);
    } else {
        emit panoramaError("Failed to save stitched image.");
    }

    m_state = IDLE;
    setRunning(false);
    setProgress(100);
}

void PanoramaBuilder::setRunning(bool r) {
    if (m_isRunning != r) {
        m_isRunning = r;
        emit isRunningChanged();
    }
}

void PanoramaBuilder::setProgress(int p) {
    if (m_progressPercent != p) {
        m_progressPercent = p;
        emit progressPercentChanged();
    }
}

void PanoramaBuilder::setLastResultPath(const QString& p) {
    if (m_lastResultPath != p) {
        m_lastResultPath = p;
        emit lastResultPathChanged();
    }
}
