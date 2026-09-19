#pragma once

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QList>
#include <QElapsedTimer>
#include <opencv2/core.hpp>
#include <opencv2/stitching.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>
#include "../network/CommandEmitter.hpp"
#include "../network/TelemetryClient.hpp"
#include "../network/VideoManager.hpp"

class PanoramaBuilder : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int progressPercent READ progressPercent NOTIFY progressPercentChanged)
    Q_PROPERTY(int totalCaptured READ totalCaptured NOTIFY totalCapturedChanged)
    Q_PROPERTY(QString lastResultPath READ lastResultPath NOTIFY lastResultPathChanged)
    Q_PROPERTY(int stepDegrees READ stepDegrees WRITE setStepDegrees NOTIFY stepDegreesChanged)
    Q_PROPERTY(int turnThrottle READ turnThrottle WRITE setTurnThrottle NOTIFY turnThrottleChanged)

public:
    explicit PanoramaBuilder(CommandEmitter* emitter, TelemetryClient* telemetry, VideoManager* video, QObject *parent = nullptr);

    bool isRunning() const { return m_isRunning; }
    int progressPercent() const { return m_progressPercent; }
    int totalCaptured() const { return m_capturedImages.size(); }
    QString lastResultPath() const { return m_lastResultPath; }
    int stepDegrees() const { return m_stepDegrees; }
    void setStepDegrees(int deg);
    int turnThrottle() const { return m_turnThrottle; }
    void setTurnThrottle(int t);

    Q_INVOKABLE void startPanorama();
    Q_INVOKABLE void cancelPanorama();

signals:
    void isRunningChanged();
    void progressPercentChanged();
    void totalCapturedChanged();
    void lastResultPathChanged();
    void stepDegreesChanged();
    void turnThrottleChanged();
    void panoramaFinished(const QString& path);
    void panoramaError(const QString& msg);

private slots:
    void onTelemetryUpdated();
    void onTurnPulseTimeout();
    void onTurnSettleTimeout();
    void stabilizeAndCapture();
    void processStitchingAsync();

private:
    void setRunning(bool r);
    void setProgress(int p);
    void setLastResultPath(const QString& p);
    void startTurnToHeading(float targetHeading);
    void issueTurnPulse();

    static float normalizeAngle(float angle);
    static float angleDiff(float from, float to);

    // Feature-mapping pairwise OpenCV stitcher engine
    cv::Mat featureBasedStitch(const std::vector<cv::Mat>& images);

    CommandEmitter*  m_emitter;
    TelemetryClient* m_telemetry;
    VideoManager*    m_video;

    enum State {
        IDLE,
        TURNING_TO_TARGET,
        STABILIZING,
        STITCHING
    };

    State   m_state;
    bool    m_isRunning;
    int     m_progressPercent;
    QString m_lastResultPath;
    
    int     m_stepDegrees;
    int     m_targetTotalShots;
    int     m_currentShotIndex;
    int     m_turnThrottle;
    
    float   m_startHeading;
    float   m_targetHeading;

    QTimer  m_turnPulseTimer;
    QTimer  m_turnSettleTimer;
    QTimer  m_stabilizeTimer;
    QElapsedTimer m_turnStepElapsed;

    QList<QImage> m_capturedImages;

    static constexpr float kTolerance   = 4.0f;   // degrees deadband
    static constexpr int   kPulseMs     = 350;    // ms turning pulse burst
    static constexpr int   kSettleMs    = 500;    // ms chassis & compass settle pause
    static constexpr int   kMinTurnPWM  = 358;    // min 35% PWM out of 1023
    static constexpr int   kMaxTurnPWM  = 512;    // max 50% PWM out of 1023
    static constexpr int   kTurnTimeoutMs = 12000; // safety max timeout per step
};
