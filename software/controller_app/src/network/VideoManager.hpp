#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QDir>
#include <QProcess>

class VideoManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(QString currentFrameBase64 READ currentFrameBase64 NOTIFY frameReceived)
    
public:
    explicit VideoManager(QObject *parent = nullptr);
    bool isRecording() const { return m_isRecording; }
    QString currentFrameBase64() const { return m_currentFrameBase64; }

public slots:
    void startStream(const QString& ip);
    void stopStream();
    void toggleRecording();

signals:
    void recordingChanged();
    void frameReceived();
    void errorOccurred(const QString& errorMsg);
    void recordingSaved(const QString& path);

private slots:
    void fetchNextFrame();
    void onFrameDownloaded();
    void compileVideo();

private:
    QNetworkAccessManager* m_nam;
    QNetworkReply* m_reply;
    QString m_cameraUrl;
    bool m_isRecording;
    QString m_currentFrameBase64;
    int m_frameCount;
    QString m_recordDir;
    QTimer* m_timer;
};
