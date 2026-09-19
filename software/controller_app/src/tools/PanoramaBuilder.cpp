#include "PanoramaBuilder.hpp"
#include <QPainter>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <cmath>

PanoramaBuilder::PanoramaBuilder(CommandEmitter* emitter, TelemetryClient* telemetry, VideoManager* video, QObject *parent)
    : QObject(parent)
    , m_emitter(emitter)
    , m_telemetry(telemetry)
    , m_video(video)
    , m_state(IDLE)
    , m_isRunning(false)
    , m_progressPercent(0)
    , m_stepDegrees(20)
    , m_targetTotalShots(19)
    , m_currentShotIndex(0)
    , m_turnThrottle(400)
    , m_startHeading(0.0f)
    , m_targetHeading(0.0f)
{
    m_stabilizeTimer.setSingleShot(true);
    connect(&m_stabilizeTimer, &QTimer::timeout, this, &PanoramaBuilder::stabilizeAndCapture);

    m_turnPulseTimer.setSingleShot(true);
    connect(&m_turnPulseTimer, &QTimer::timeout, this, &PanoramaBuilder::onTurnPulseTimeout);

    m_turnSettleTimer.setSingleShot(true);
    connect(&m_turnSettleTimer, &QTimer::timeout, this, &PanoramaBuilder::onTurnSettleTimeout);

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

    m_stepDegrees = qBound(10, m_stepDegrees, 90);
    // Add +1 shot so the 360° rotation closes the loop and overlaps back onto the starting image
    int baseShots = static_cast<int>(std::ceil(360.0f / m_stepDegrees));
    m_targetTotalShots = baseShots + 1;
    m_currentShotIndex = 0;

    m_startHeading = m_telemetry->headingCompassDeg();
    m_targetHeading = m_startHeading;

    setRunning(true);
    setProgress(0);
    setLastResultPath("");

    qDebug() << "PanoramaBuilder: Starting full 360 panorama, startHeading=" << m_startHeading
             << "stepDegrees=" << m_stepDegrees << "totalShots=" << m_targetTotalShots;

    // Capture first frame (shot 0) at current heading
    m_state = STABILIZING;
    m_stabilizeTimer.start(kSettleMs);
}

void PanoramaBuilder::cancelPanorama() {
    if (!m_isRunning) return;

    m_stabilizeTimer.stop();
    m_turnPulseTimer.stop();
    m_turnSettleTimer.stop();

    if (m_emitter) {
        m_emitter->updateSteering(0);
        m_emitter->updateThrottle(0);
    }

    m_state = IDLE;
    setRunning(false);
    setProgress(0);
    emit panoramaError("Panorama cancelled by user.");
}

void PanoramaBuilder::startTurnToHeading(float targetHeading) {
    if (!m_isRunning || !m_telemetry || !m_emitter) return;

    m_targetHeading = targetHeading;
    m_state = TURNING_TO_TARGET;
    m_turnStepElapsed.start();

    qDebug() << "PanoramaBuilder: Turning to angle targetHeading=" << m_targetHeading
             << "for shot index" << m_currentShotIndex;

    issueTurnPulse();
}

void PanoramaBuilder::issueTurnPulse() {
    if (!m_isRunning || m_state != TURNING_TO_TARGET || !m_telemetry || !m_emitter) return;

    // Safety timeout check
    if (m_turnStepElapsed.elapsed() > kTurnTimeoutMs) {
        qWarning() << "PanoramaBuilder: Turn step timed out for target" << m_targetHeading;
        m_emitter->updateSteering(0);
        m_emitter->updateThrottle(0);
        // Advance to capture phase anyway so panorama does not freeze
        m_state = STABILIZING;
        m_stabilizeTimer.start(kSettleMs);
        return;
    }

    float current = m_telemetry->headingCompassDeg();
    float diff    = angleDiff(current, m_targetHeading);

    // Reached target heading?
    if (std::abs(diff) <= kTolerance) {
        qDebug() << "PanoramaBuilder: Reached target angle" << m_targetHeading << "heading=" << current;
        m_emitter->updateSteering(0);
        m_emitter->updateThrottle(0);
        m_state = STABILIZING;
        m_stabilizeTimer.start(kSettleMs);
        return;
    }

    // Proportional turn PWM speed scaling between min 35% (358) and max 50% (512)
    float scale = std::abs(diff) / 180.0f;
    int speed = static_cast<int>(kMinTurnPWM + scale * (kMaxTurnPWM - kMinTurnPWM));
    speed = qBound(kMinTurnPWM, speed, kMaxTurnPWM);

    int steerVal = (diff > 0) ? speed : -speed;

    m_emitter->updateThrottle(0);
    m_emitter->updateSteering(steerVal);

    m_turnPulseTimer.start(kPulseMs);
}

void PanoramaBuilder::onTurnPulseTimeout() {
    if (!m_isRunning || m_state != TURNING_TO_TARGET) return;

    // Stop motors during settle window
    if (m_emitter) {
        m_emitter->updateSteering(0);
        m_emitter->updateThrottle(0);
    }

    m_turnSettleTimer.start(kSettleMs);
}

void PanoramaBuilder::onTurnSettleTimeout() {
    if (!m_isRunning || m_state != TURNING_TO_TARGET) return;
    issueTurnPulse();
}

void PanoramaBuilder::onTelemetryUpdated() {
    // Heading checks handled via pulse-settle loop
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
            
            setProgress((m_capturedImages.size() * 90) / m_targetTotalShots);
            qDebug() << "PanoramaBuilder: Captured shot" << m_capturedImages.size() << "/" << m_targetTotalShots;
        } else {
            qWarning() << "PanoramaBuilder: Failed to decode image from base64 data.";
        }
    } else {
        qWarning() << "PanoramaBuilder: Failed to capture video frame.";
        cancelPanorama();
        emit panoramaError("Failed to capture video frame.");
        return;
    }

    m_currentShotIndex++;

    if (m_currentShotIndex >= m_targetTotalShots) {
        qDebug() << "PanoramaBuilder: All shots captured! Starting OpenCV feature stitching...";
        if (m_emitter) {
            m_emitter->updateSteering(0);
            m_emitter->updateThrottle(0);
        }
        processStitchingAsync();
    } else {
        float nextTarget = normalizeAngle(m_startHeading + m_currentShotIndex * m_stepDegrees);
        startTurnToHeading(nextTarget);
    }
}

cv::Mat PanoramaBuilder::featureBasedStitch(const std::vector<cv::Mat>& images) {
    if (images.empty()) return cv::Mat();
    if (images.size() == 1) return images[0].clone();

    // Primary: OpenCV Stitcher API
    try {
        cv::Mat resultMat;
        cv::Ptr<cv::Stitcher> stitcher = cv::Stitcher::create(cv::Stitcher::PANORAMA);
        stitcher->setFeaturesFinder(cv::ORB::create(2000));
        cv::Stitcher::Status status = stitcher->stitch(images, resultMat);
        
        std::vector<int> component = stitcher->component();
        
        if (status == cv::Stitcher::OK && !resultMat.empty() && component.size() == images.size()) {
            qDebug() << "PanoramaBuilder: OpenCV cv::Stitcher successfully stitched all" << images.size() << "images!";
            return resultMat;
        }
        qWarning() << "PanoramaBuilder: cv::Stitcher returned status:" << status 
                   << "used images:" << component.size() << "/" << images.size()
                   << "- running kinematic fallback...";
    } catch (const cv::Exception& e) {
        qWarning() << "PanoramaBuilder: cv::Stitcher exception:" << e.what();
    }

    // Secondary: Kinematic Translation Blending based on commanded step degrees
    // ESP32-CAM OV2640 horizontal FOV is approx 65 degrees.
    float fov = 65.0f;
    int imgW = images[0].cols;
    int imgH = images[0].rows;
    int shiftPixels = static_cast<int>((m_stepDegrees / fov) * imgW);
    
    // Fallback bounds check
    if (shiftPixels < 1) shiftPixels = 1;
    if (shiftPixels > imgW) shiftPixels = imgW;

    int totalW = shiftPixels * (images.size() - 1) + imgW;
    cv::Mat canvas(imgH, totalW, images[0].type(), cv::Scalar(0, 0, 0));

    for (size_t i = 0; i < images.size(); ++i) {
        int xOffset = i * shiftPixels;
        
        for (int y = 0; y < imgH; ++y) {
            for (int x = 0; x < imgW; ++x) {
                cv::Vec3b newPix = images[i].at<cv::Vec3b>(y, x);
                cv::Vec3b& canvasPix = canvas.at<cv::Vec3b>(y, xOffset + x);
                
                if (canvasPix == cv::Vec3b(0, 0, 0)) {
                    canvasPix = newPix;
                } else {
                    // Cross-fade blending based on horizontal position to avoid ghosting
                    float alpha = 1.0f;
                    int overlapWidth = imgW - shiftPixels;
                    if (overlapWidth > 0) {
                        alpha = static_cast<float>(x) / overlapWidth;
                        if (alpha > 1.0f) alpha = 1.0f;
                        if (alpha < 0.0f) alpha = 0.0f;
                    }
                    
                    canvasPix = cv::Vec3b(
                        static_cast<uchar>(canvasPix[0] * (1.0f - alpha) + newPix[0] * alpha),
                        static_cast<uchar>(canvasPix[1] * (1.0f - alpha) + newPix[1] * alpha),
                        static_cast<uchar>(canvasPix[2] * (1.0f - alpha) + newPix[2] * alpha)
                    );
                }
            }
        }
    }

    return canvas;
}

void PanoramaBuilder::processStitchingAsync() {
    m_state = STITCHING;
    setProgress(92);

    if (m_capturedImages.isEmpty()) {
        cancelPanorama();
        return;
    }

    std::vector<cv::Mat> cvImages;
    for (const QImage& qimg : m_capturedImages) {
        QImage rgb = qimg.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat(rgb.height(), rgb.width(), CV_8UC3, (void*)rgb.constBits(), rgb.bytesPerLine());
        cv::Mat bgrMat;
        cv::cvtColor(mat, bgrMat, cv::COLOR_RGB2BGR);
        cvImages.push_back(bgrMat.clone());
    }

    auto watcher = new QFutureWatcher<cv::Mat>(this);
    connect(watcher, &QFutureWatcher<cv::Mat>::finished, this, [this, watcher]() {
        cv::Mat resultMat = watcher->result();
        watcher->deleteLater();

        if (!resultMat.empty()) {
            QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            QDir dir(docsPath);
            if (!dir.exists("LibreESP")) {
                dir.mkpath("LibreESP");
            }
            QString fileName = QString("Panorama_%1.jpg").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
            QString fullPath = dir.filePath("LibreESP/" + fileName);

            if (cv::imwrite(fullPath.toStdString(), resultMat)) {
                setLastResultPath(fullPath);
                emit panoramaFinished(fullPath);
            } else {
                emit panoramaError("Failed to save OpenCV stitched image to disk.");
            }
        } else {
            emit panoramaError("OpenCV feature stitching failed.");
        }

        m_state = IDLE;
        setRunning(false);
        setProgress(100);
    });

    QFuture<cv::Mat> future = QtConcurrent::run([this, cvImages]() {
        return featureBasedStitch(cvImages);
    });
    watcher->setFuture(future);
}

float PanoramaBuilder::normalizeAngle(float angle) {
    float a = std::fmod(angle, 360.0f);
    if (a < 0.0f) a += 360.0f;
    return a;
}

float PanoramaBuilder::angleDiff(float from, float to) {
    float diff = to - from;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff <= -180.0f) diff += 360.0f;
    return diff;
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
