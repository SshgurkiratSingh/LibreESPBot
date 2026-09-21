#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

class AiToolDispatcher : public QObject {
    Q_OBJECT
public:
    explicit AiToolDispatcher(QObject* parent = nullptr);

public slots:
    void dispatchTool(const QString& name, const QJsonObject& args);

signals:
    void executeScriptRequested(QString script);
    void stopRoverRequested();
    void saveImageRequested(QString label);
    void saveImageToMemoryRequested(QString key);
    void recallImageRequested(QString key);
    void describeViewRequested(QString key);
    void setHeadlightRequested(int mode);
    void setServoRequested(int angle);
    void sweepRadarRequested(int speed);
    void waitForTelemetryRequested(QString condition, int timeout_ms);
    void speakRequested(QString text);
    void logNoteRequested(QString text);
    void takePanoramaRequested();
    void askUserRequested(QString question);
    void loopEnableRequested(bool on);
    void generateReportRequested(QString title, QString content, QStringList imageKeys);
    void rememberRequested(QString text);
    void recallMemoryRequested(QString query);
};
