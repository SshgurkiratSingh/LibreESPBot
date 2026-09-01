#pragma once

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QList>
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
    void stabilizeAndCapture();
    void stitchImages();

private:
    void setRunning(bool r);
    void setProgress(int p);
    void setLastResultPath(const QString& p);
    void executeNextTurn();
    float normalizeAngle(float angle);
    float angleDifference(float target, float current);

    CommandEmitter* m_emitter;
    TelemetryClient* m_telemetry;
    VideoManager* m_video;

    enum State {
        IDLE,
        ROTATING,
        STABILIZING,
        STITCHING
    };

    State m_state;
    bool m_isRunning;
    int m_progressPercent;
    QString m_lastResultPath;
    
    int m_stepDegrees;
    int m_targetTotalShots;
    int m_turnThrottle;
    
    float m_initialYaw;
    float m_targetYaw;
    float m_totalTurned;
    
    QList<QImage> m_capturedImages;
    QTimer m_stabilizeTimer;
};
