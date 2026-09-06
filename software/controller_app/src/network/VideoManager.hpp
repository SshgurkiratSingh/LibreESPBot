#pragma once
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

class CommandEmitter;

class VideoManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(QString currentFrameBase64 READ currentFrameBase64 NOTIFY frameReceived)
    Q_PROPERTY(int targetFps READ targetFps WRITE setTargetFps NOTIFY targetFpsChanged)
    
    // OpenCV Features
    Q_PROPERTY(bool cvCrosshair READ cvCrosshair WRITE setCvCrosshair NOTIFY cvSettingsChanged)
    Q_PROPERTY(bool cvEdgeDetection READ cvEdgeDetection WRITE setCvEdgeDetection NOTIFY cvSettingsChanged)
    Q_PROPERTY(bool cvNightVision READ cvNightVision WRITE setCvNightVision NOTIFY cvSettingsChanged)
    Q_PROPERTY(bool cvGrayscale READ cvGrayscale WRITE setCvGrayscale NOTIFY cvSettingsChanged)
    Q_PROPERTY(bool cvGaussianBlur READ cvGaussianBlur WRITE setCvGaussianBlur NOTIFY cvSettingsChanged)
    Q_PROPERTY(bool cvInvertColors READ cvInvertColors WRITE setCvInvertColors NOTIFY cvSettingsChanged)
    
    // Tracking Features
    Q_PROPERTY(bool cvAutoFollow READ cvAutoFollow WRITE setCvAutoFollow NOTIFY cvSettingsChanged)
    Q_PROPERTY(int cvTrackHue READ cvTrackHue WRITE setCvTrackHue NOTIFY cvSettingsChanged)
    Q_PROPERTY(bool cvPickColorActive READ cvPickColorActive WRITE setCvPickColorActive NOTIFY cvSettingsChanged)
    Q_PROPERTY(bool cvMotionTracking READ cvMotionTracking WRITE setCvMotionTracking NOTIFY cvSettingsChanged)
    Q_PROPERTY(bool cvFaceTracking READ cvFaceTracking WRITE setCvFaceTracking NOTIFY cvSettingsChanged)
    Q_PROPERTY(bool cvAutoDrive READ cvAutoDrive WRITE setCvAutoDrive NOTIFY cvSettingsChanged)
    
public:
    explicit VideoManager(QObject *parent = nullptr);
    void setCommandEmitter(CommandEmitter* emitter) { m_commandEmitter = emitter; }
    
    Q_INVOKABLE void requestColorPick(double xRatio, double yRatio);
    
    bool isRecording() const { return m_isRecording; }
    QString currentFrameBase64() const { return m_currentFrameBase64; }
    int targetFps() const { return m_targetFps; }
    void setTargetFps(int fps);

    bool cvCrosshair() const { return m_cvCrosshair; }
    void setCvCrosshair(bool enabled);
    
    bool cvEdgeDetection() const { return m_cvEdgeDetection; }
    void setCvEdgeDetection(bool enabled);
    
    bool cvNightVision() const { return m_cvNightVision; }
    void setCvNightVision(bool enabled);
    
    bool cvGrayscale() const { return m_cvGrayscale; }
    void setCvGrayscale(bool enabled);
    
    bool cvGaussianBlur() const { return m_cvGaussianBlur; }
    void setCvGaussianBlur(bool enabled);
    
    bool cvInvertColors() const { return m_cvInvertColors; }
    void setCvInvertColors(bool enabled);

    bool cvAutoFollow() const { return m_cvAutoFollow; }
    void setCvAutoFollow(bool enabled);
    
    int cvTrackHue() const { return m_cvTrackHue; }
    void setCvTrackHue(int hue);
    
    bool cvPickColorActive() const { return m_cvPickColorActive; }
    void setCvPickColorActive(bool enabled);
    
    bool cvMotionTracking() const { return m_cvMotionTracking; }
    void setCvMotionTracking(bool enabled);
    
    bool cvFaceTracking() const { return m_cvFaceTracking; }
    void setCvFaceTracking(bool enabled);
    
    bool cvAutoDrive() const { return m_cvAutoDrive; }
    void setCvAutoDrive(bool enabled);

public slots:
    void startStream(const QString& ip);
    void stopStream();
    void toggleRecording();

signals:
    void frameReceived();
    void recordingChanged();
    void targetFpsChanged();
    void errorOccurred(const QString &error);
    void recordingSaved(const QString &path);
    void cvSettingsChanged();

private slots:
    void onFrameDownloaded();

private:
    void fetchNextFrame();
    void compileVideo();

    QNetworkAccessManager *m_nam;
    QNetworkReply *m_reply;
    QTimer *m_timer;
    QString m_cameraUrl;
    CommandEmitter* m_commandEmitter = nullptr;
    
    int m_targetFps = 15;
    bool m_isRecording = false;
    QString m_recordDir;
    int m_frameCount = 0;
    QString m_currentFrameBase64;
    
    bool m_cvCrosshair;
    bool m_cvEdgeDetection;
    bool m_cvNightVision;
    bool m_cvGrayscale;
    bool m_cvGaussianBlur;
    bool m_cvInvertColors;
    
    bool m_cvAutoFollow = false;
    bool m_cvMotionTracking = false;
    bool m_cvFaceTracking = false;
    bool m_cvAutoDrive = false;
    int m_cvTrackHue = 0; // 0 = Red
    
    cv::Mat m_prevGrayFrame;
    cv::CascadeClassifier m_faceCascade;
    bool m_faceCascadeLoaded = false;
    
    bool m_cvPickColorActive = false;
    bool m_needsColorPick = false;
    double m_pickX = 0.0;
    double m_pickY = 0.0;
};
