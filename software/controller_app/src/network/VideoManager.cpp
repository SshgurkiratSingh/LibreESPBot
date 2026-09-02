#include "VideoManager.hpp"
#include <QNetworkRequest>
#include <QImage>
#include <QBuffer>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QtConcurrent>

VideoManager::VideoManager(QObject *parent) 
    : QObject(parent), m_reply(nullptr), m_isRecording(false), m_frameCount(0), m_targetFps(10) {
    m_timer = new QTimer(this);
    m_nam = new QNetworkAccessManager(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &VideoManager::fetchNextFrame);
}

void VideoManager::startStream(const QString& ip) {
    m_cameraUrl = "http://" + ip + "/capture";
    if (!m_timer->isActive()) {
        fetchNextFrame();
    }
}

void VideoManager::stopStream() {
    m_timer->stop();
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void VideoManager::toggleRecording() {
    if (m_isRecording) {
        m_isRecording = false;
        emit recordingChanged();
        compileVideo();
    } else {
        QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        m_recordDir = docsPath + "/LibreESPBot_Records/" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QDir().mkpath(m_recordDir);
        m_frameCount = 0;
        m_isRecording = true;
        emit recordingChanged();
        qDebug() << "Started recording to:" << m_recordDir;
    }
}

void VideoManager::setTargetFps(int fps) {
    if (m_targetFps != fps && fps > 0) {
        m_targetFps = fps;
        emit targetFpsChanged();
    }
}

void VideoManager::fetchNextFrame() {
    if (m_cameraUrl.isEmpty()) return;

    QNetworkRequest request(QUrl(m_cameraUrl + "?t=" + QString::number(QDateTime::currentMSecsSinceEpoch())));
    // Set aggressive timeout
    request.setTransferTimeout(1000);
    
    if (m_reply) {
        m_reply->deleteLater();
    }
    
    m_reply = m_nam->get(request);
    connect(m_reply, &QNetworkReply::finished, this, &VideoManager::onFrameDownloaded);
}

void VideoManager::onFrameDownloaded() {
    if (!m_reply) return;
    
    if (m_reply->error() == QNetworkReply::NoError) {
        QByteArray jpegData = m_reply->readAll();
        
        // Copy capture context for worker
        bool isRec = m_isRecording;
        QString recDir = m_recordDir;
        int frameNum = m_frameCount;
        if (isRec) m_frameCount++; // Increment on main thread safely
        
        // Offload heavy operations (Base64 encoding & Disk IO) to a background thread pool
        QtConcurrent::run([this, jpegData, isRec, recDir, frameNum]() {
            // Encode Base64
            QString base64 = "data:image/jpeg;base64," + QString(jpegData.toBase64());
            
            // Save frame if recording
            if (isRec && !recDir.isEmpty()) {
                QString framePath = recDir + QString("/frame_%1.jpg").arg(frameNum, 5, 10, QChar('0'));
                QFile file(framePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(jpegData);
                    file.close();
                }
            }
            
            // Marshall the UI update back to the Main GUI thread safely
            QMetaObject::invokeMethod(this, [this, base64]() {
                m_currentFrameBase64 = base64;
                emit frameReceived();
            }, Qt::QueuedConnection);
        });
        
    } else {
        qWarning() << "VideoManager fetch error:" << m_reply->errorString();
        emit errorOccurred(m_reply->errorString());
    }
    
    m_reply->deleteLater();
    m_reply = nullptr;
    
    // Poll next frame at target FPS
    int interval = 1000 / m_targetFps;
    m_timer->start(interval);
}

void VideoManager::compileVideo() {
    if (m_frameCount == 0 || m_recordDir.isEmpty()) return;
    
    QString outputPath = m_recordDir + "_video.mp4";
    qDebug() << "Compiling video to:" << outputPath;
    
    QProcess *process = new QProcess(this);
    QStringList args;
    args << "-framerate" << "10" 
         << "-i" << m_recordDir + "/frame_%05d.jpg" 
         << "-c:v" << "copy" 
         << outputPath;
         
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
            [this, process, outputPath](int exitCode, QProcess::ExitStatus status) {
        if (exitCode == 0) {
            emit recordingSaved(outputPath);
            qDebug() << "Video compiled successfully!";
            // Clean up raw frames
            QDir dir(m_recordDir);
            dir.removeRecursively();
        } else {
            qWarning() << "FFmpeg compilation failed!";
            emit errorOccurred("FFmpeg failed to compile video.");
        }
        process->deleteLater();
    });
    
    process->start("ffmpeg", args);
}
