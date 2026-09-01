#pragma once

#include <QObject>
#include <QTimer>
#include "../network/CommandEmitter.hpp"
#include "../network/TelemetryClient.hpp"

class TurningCalibrator : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int currentTestThrottle READ currentTestThrottle NOTIFY currentTestThrottleChanged)
    Q_PROPERTY(int optimalThrottle READ optimalThrottle NOTIFY optimalThrottleChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit TurningCalibrator(CommandEmitter* emitter, TelemetryClient* telemetry, QObject *parent = nullptr);

    bool isRunning() const { return m_isRunning; }
    int currentTestThrottle() const { return m_currentTestThrottle; }
    int optimalThrottle() const { return m_optimalThrottle; }
    QString statusMessage() const { return m_statusMessage; }

    Q_INVOKABLE void startCalibration();
    Q_INVOKABLE void cancelCalibration();

signals:
    void isRunningChanged();
    void currentTestThrottleChanged();
    void optimalThrottleChanged();
    void statusMessageChanged();
    void calibrationFinished(int optimalThrottle);

private slots:
    void stopAndEvaluate();

private:
    void setRunning(bool r);
    void setStatus(const QString& msg);
    void runNextTest();
    float angleDifference(float target, float current);

    CommandEmitter* m_emitter;
    TelemetryClient* m_telemetry;

    bool m_isRunning;
    int m_currentTestThrottle;
    int m_optimalThrottle;
    QString m_statusMessage;
    
    float m_initialYaw;
    QTimer m_testTimer;
};
