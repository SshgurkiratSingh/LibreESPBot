#include "VideoManager.hpp"
#include <QNetworkRequest>
#include <QImage>
#include <QBuffer>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QtConcurrent>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

#include "CommandEmitter.hpp"

VideoManager::VideoManager(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)), m_reply(nullptr),
      m_isRecording(false), m_frameCount(0), m_targetFps(15),
      m_cvCrosshair(true), m_cvEdgeDetection(false), m_cvNightVision(false),
      m_cvGrayscale(false), m_cvGaussianBlur(false), m_cvInvertColors(false),
      m_cvAutoFollow(false), m_cvTrackHue(0)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &VideoManager::fetchNextFrame);
    
    m_faceCascadeLoaded = m_faceCascade.load("haarcascade_frontalface_default.xml");
    if (!m_faceCascadeLoaded) {
        qWarning() << "Failed to load haarcascade_frontalface_default.xml";
    }
}

void VideoManager::setCvCrosshair(bool enabled) { if (m_cvCrosshair != enabled) { m_cvCrosshair = enabled; emit cvSettingsChanged(); } }
void VideoManager::setCvEdgeDetection(bool enabled) { if (m_cvEdgeDetection != enabled) { m_cvEdgeDetection = enabled; emit cvSettingsChanged(); } }
void VideoManager::setCvNightVision(bool enabled) { if (m_cvNightVision != enabled) { m_cvNightVision = enabled; emit cvSettingsChanged(); } }
void VideoManager::setCvGrayscale(bool enabled) { if (m_cvGrayscale != enabled) { m_cvGrayscale = enabled; emit cvSettingsChanged(); } }
void VideoManager::setCvGaussianBlur(bool enabled) { if (m_cvGaussianBlur != enabled) { m_cvGaussianBlur = enabled; emit cvSettingsChanged(); } }
void VideoManager::setCvInvertColors(bool enabled) { if (m_cvInvertColors != enabled) { m_cvInvertColors = enabled; emit cvSettingsChanged(); } }

void VideoManager::setCvAutoFollow(bool enabled) { if (m_cvAutoFollow != enabled) { m_cvAutoFollow = enabled; emit cvSettingsChanged(); } }
void VideoManager::setCvMotionTracking(bool enabled) { if (m_cvMotionTracking != enabled) { m_cvMotionTracking = enabled; emit cvSettingsChanged(); } }
void VideoManager::setCvFaceTracking(bool enabled) { if (m_cvFaceTracking != enabled) { m_cvFaceTracking = enabled; emit cvSettingsChanged(); } }
void VideoManager::setCvAutoDrive(bool enabled) { if (m_cvAutoDrive != enabled) { m_cvAutoDrive = enabled; emit cvSettingsChanged(); } }
void VideoManager::setCvTrackHue(int hue) { if (m_cvTrackHue != hue) { m_cvTrackHue = hue; emit cvSettingsChanged(); } }
void VideoManager::setCvPickColorActive(bool enabled) { if (m_cvPickColorActive != enabled) { m_cvPickColorActive = enabled; emit cvSettingsChanged(); } }

void VideoManager::requestColorPick(double xRatio, double yRatio) {
    if (m_cvPickColorActive) {
        m_pickX = xRatio;
        m_pickY = yRatio;
        m_needsColorPick = true;
    }
}
void VideoManager::setTargetFps(int fps) {
    if (m_targetFps != fps && fps > 0) {
        m_targetFps = fps;
        emit targetFpsChanged();
    }
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
        
        // Capture OpenCV flags
        bool doCrosshair = m_cvCrosshair;
        bool doEdge = m_cvEdgeDetection;
        bool doNightVision = m_cvNightVision;
        bool doGray = m_cvGrayscale;
        bool doBlur = m_cvGaussianBlur;
        bool doInvert = m_cvInvertColors;
        bool doAutoFollow = m_cvAutoFollow;
        int trackHue = m_cvTrackHue;
        CommandEmitter* emitter = m_commandEmitter;
        
        bool needsColorPick = m_needsColorPick;
        double pickX = m_pickX;
        double pickY = m_pickY;
        m_needsColorPick = false; // reset immediately
        bool doMotionTracking = m_cvMotionTracking;
        cv::Mat prevGrayFrame = m_prevGrayFrame;
        bool doAutoDrive = m_cvAutoDrive;
        bool doFaceTracking = m_cvFaceTracking;
        bool cascadeLoaded = m_faceCascadeLoaded;
        
        // Offload heavy operations (Base64 encoding & Disk IO) to a background thread pool
        (void)QtConcurrent::run([this, jpegData, isRec, recDir, frameNum, doCrosshair, doEdge, doNightVision, doGray, doBlur, doInvert, doAutoFollow, trackHue, emitter, needsColorPick, pickX, pickY, doMotionTracking, prevGrayFrame, doAutoDrive, doFaceTracking, cascadeLoaded]() {
            // Decode JPEG with OpenCV
            std::vector<uchar> buffer(jpegData.begin(), jpegData.end());
            cv::Mat frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
            
            if (!frame.empty()) {
                cv::Mat currentGray;
                if (doMotionTracking) {
                    cv::cvtColor(frame, currentGray, cv::COLOR_BGR2GRAY);
                    cv::GaussianBlur(currentGray, currentGray, cv::Size(21, 21), 0);
                    
                    if (!prevGrayFrame.empty()) {
                        cv::Mat frameDelta, thresh;
                        cv::absdiff(prevGrayFrame, currentGray, frameDelta);
                        cv::threshold(frameDelta, thresh, 25, 255, cv::THRESH_BINARY);
                        cv::dilate(thresh, thresh, cv::Mat(), cv::Point(-1, -1), 2);
                        
                        std::vector<std::vector<cv::Point>> contours;
                        cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
                        
                        double maxArea = 0;
                        int largestContourIdx = -1;
                        for (size_t i = 0; i < contours.size(); i++) {
                            double area = cv::contourArea(contours[i]);
                            if (area > maxArea) {
                                maxArea = area;
                                largestContourIdx = i;
                            }
                        }
                        
                        if (largestContourIdx >= 0 && maxArea > 500) {
                            cv::Rect boundingRect = cv::boundingRect(contours[largestContourIdx]);
                            cv::rectangle(frame, boundingRect, cv::Scalar(0, 255, 255), 2);
                            
                            int cx = boundingRect.x + boundingRect.width / 2;
                            int cy = boundingRect.y + boundingRect.height / 2;
                            cv::circle(frame, cv::Point(cx, cy), 5, cv::Scalar(0, 0, 255), -1);
                            
                            int frameCenter = frame.cols / 2;
                            int error = cx - frameCenter;
                            int steering = (error * 32767) / frameCenter;
                            
                            if (steering > 32767) steering = 32767;
                            if (steering < -32767) steering = -32767;
                            
                            double targetArea = frame.cols * frame.rows * 0.15;
                            int throttle = 0;
                            if (maxArea < targetArea * 0.7) {
                                throttle = 25000;
                            } else if (maxArea > targetArea * 1.3) {
                                throttle = -25000;
                            }
                            
                            if (emitter && doAutoDrive) {
                                QMetaObject::invokeMethod(emitter, "updateSteering", Qt::QueuedConnection, Q_ARG(int16_t, steering));
                                QMetaObject::invokeMethod(emitter, "updateThrottle", Qt::QueuedConnection, Q_ARG(int16_t, throttle));
                            }
                            
                            cv::putText(frame, "MOTION LOCKED", cv::Point(10, frame.rows - 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
                        } else {
                            if (emitter && doAutoDrive) {
                                QMetaObject::invokeMethod(emitter, "updateSteering", Qt::QueuedConnection, Q_ARG(int16_t, 0));
                                QMetaObject::invokeMethod(emitter, "updateThrottle", Qt::QueuedConnection, Q_ARG(int16_t, 0));
                            }
                            cv::putText(frame, "SEARCHING MOTION...", cv::Point(10, frame.rows - 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 165, 255), 2);
                        }
                    } else {
                        if (emitter) {
                            QMetaObject::invokeMethod(emitter, "updateSteering", Qt::QueuedConnection, Q_ARG(int16_t, 0));
                            QMetaObject::invokeMethod(emitter, "updateThrottle", Qt::QueuedConnection, Q_ARG(int16_t, 0));
                        }
                        cv::putText(frame, "INIT MOTION...", cv::Point(10, frame.rows - 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 165, 255), 2);
                    }
                }

                if (needsColorPick) {
                    int x = qBound(0, static_cast<int>(pickX * frame.cols), frame.cols - 1);
                    int y = qBound(0, static_cast<int>(pickY * frame.rows), frame.rows - 1);
                    cv::Mat hsv;
                    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
                    cv::Vec3b pixel = hsv.at<cv::Vec3b>(y, x);
                    int pickedHue = pixel[0]; // Hue is first channel
                    
                    QMetaObject::invokeMethod(this, [this, pickedHue]() {
                        setCvTrackHue(pickedHue);
                        setCvPickColorActive(false);
                    }, Qt::QueuedConnection);
                }
                
                if (doAutoFollow) {
                    cv::Mat hsv, mask;
                    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
                    
                    int lowerHue = trackHue - 10;
                    int upperHue = trackHue + 10;
                    
                    if (lowerHue < 0) {
                        cv::Mat mask1, mask2;
                        cv::inRange(hsv, cv::Scalar(180 + lowerHue, 120, 70), cv::Scalar(180, 255, 255), mask1);
                        cv::inRange(hsv, cv::Scalar(0, 120, 70), cv::Scalar(upperHue, 255, 255), mask2);
                        mask = mask1 | mask2;
                    } else if (upperHue > 180) {
                        cv::Mat mask1, mask2;
                        cv::inRange(hsv, cv::Scalar(lowerHue, 120, 70), cv::Scalar(180, 255, 255), mask1);
                        cv::inRange(hsv, cv::Scalar(0, 120, 70), cv::Scalar(upperHue - 180, 255, 255), mask2);
                        mask = mask1 | mask2;
                    } else {
                        cv::inRange(hsv, cv::Scalar(lowerHue, 120, 70), cv::Scalar(upperHue, 255, 255), mask);
                    }
                    
                    std::vector<std::vector<cv::Point>> contours;
                    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
                    
                    double maxArea = 0;
                    int largestContourIdx = -1;
                    for (size_t i = 0; i < contours.size(); i++) {
                        double area = cv::contourArea(contours[i]);
                        if (area > maxArea) {
                            maxArea = area;
                            largestContourIdx = i;
                        }
                    }
                    
                    if (largestContourIdx >= 0 && maxArea > 500) {
                        cv::Rect boundingRect = cv::boundingRect(contours[largestContourIdx]);
                        cv::rectangle(frame, boundingRect, cv::Scalar(0, 165, 255), 2);
                        
                        int cx = boundingRect.x + boundingRect.width / 2;
                        int cy = boundingRect.y + boundingRect.height / 2;
                        cv::circle(frame, cv::Point(cx, cy), 5, cv::Scalar(0, 0, 255), -1);
                        
                        int frameCenter = frame.cols / 2;
                        int error = cx - frameCenter;
                        int steering = (error * 32767) / frameCenter;
                        
                        if (steering > 32767) steering = 32767;
                        if (steering < -32767) steering = -32767;
                        
                        double targetArea = frame.cols * frame.rows * 0.15;
                        int throttle = 0;
                        if (maxArea < targetArea * 0.7) {
                            throttle = 25000;
                        } else if (maxArea > targetArea * 1.3) {
                            throttle = -25000;
                        }
                        
                        if (emitter && doAutoDrive) {
                            QMetaObject::invokeMethod(emitter, "updateSteering", Qt::QueuedConnection, Q_ARG(int16_t, steering));
                            QMetaObject::invokeMethod(emitter, "updateThrottle", Qt::QueuedConnection, Q_ARG(int16_t, throttle));
                        }
                        
                        cv::putText(frame, "COLOR LOCKED", cv::Point(10, frame.rows - 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
                    } else {
                        if (emitter && doAutoDrive) {
                            QMetaObject::invokeMethod(emitter, "updateSteering", Qt::QueuedConnection, Q_ARG(int16_t, 0));
                            QMetaObject::invokeMethod(emitter, "updateThrottle", Qt::QueuedConnection, Q_ARG(int16_t, 0));
                        }
                        cv::putText(frame, "SEARCHING COLOR...", cv::Point(10, frame.rows - 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
                    }
                }
                
                if (doFaceTracking && cascadeLoaded) {
                    cv::Mat gray;
                    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
                    cv::equalizeHist(gray, gray);
                    
                    std::vector<cv::Rect> faces;
                    // This is safe since detectMultiScale is thread-safe for a const-like operation on a loaded cascade
                    // We const_cast or just cast away since we captured `this` which gives us access to m_faceCascade.
                    // Wait, m_faceCascade is accessible via `this->m_faceCascade`, which we can call directly.
                    this->m_faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));
                    
                    if (!faces.empty()) {
                        cv::Rect largestFace = faces[0];
                        for (const auto& face : faces) {
                            if (face.area() > largestFace.area()) largestFace = face;
                        }
                        
                        cv::rectangle(frame, largestFace, cv::Scalar(255, 0, 255), 2);
                        
                        int cx = largestFace.x + largestFace.width / 2;
                        int cy = largestFace.y + largestFace.height / 2;
                        cv::circle(frame, cv::Point(cx, cy), 5, cv::Scalar(255, 0, 255), -1);
                        
                        int frameCenter = frame.cols / 2;
                        int error = cx - frameCenter;
                        int steering = (error * 32767) / frameCenter;
                        
                        if (steering > 32767) steering = 32767;
                        if (steering < -32767) steering = -32767;
                        
                        double targetArea = frame.cols * frame.rows * 0.10;
                        int throttle = 0;
                        if (largestFace.area() < targetArea * 0.7) {
                            throttle = 25000;
                        } else if (largestFace.area() > targetArea * 1.3) {
                            throttle = -25000;
                        }
                        
                        if (emitter && doAutoDrive) {
                            QMetaObject::invokeMethod(emitter, "updateSteering", Qt::QueuedConnection, Q_ARG(int16_t, steering));
                            QMetaObject::invokeMethod(emitter, "updateThrottle", Qt::QueuedConnection, Q_ARG(int16_t, throttle));
                        }
                        
                        cv::putText(frame, "FACE LOCKED", cv::Point(10, frame.rows - 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 0, 255), 2);
                    } else {
                        if (emitter && doAutoDrive) {
                            QMetaObject::invokeMethod(emitter, "updateSteering", Qt::QueuedConnection, Q_ARG(int16_t, 0));
                            QMetaObject::invokeMethod(emitter, "updateThrottle", Qt::QueuedConnection, Q_ARG(int16_t, 0));
                        }
                        cv::putText(frame, "SEARCHING FACE...", cv::Point(10, frame.rows - 50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
                    }
                }
                
                // Apply Filters
                if (doGray) {
                    cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
                    cv::cvtColor(frame, frame, cv::COLOR_GRAY2BGR); // Convert back so downstream logic doesn't crash
                }
                if (doBlur) {
                    cv::GaussianBlur(frame, frame, cv::Size(15, 15), 0);
                }
                if (doEdge) {
                    cv::Mat edges;
                    cv::Canny(frame, edges, 50, 150);
                    cv::cvtColor(edges, frame, cv::COLOR_GRAY2BGR);
                }
                if (doInvert) {
                    cv::bitwise_not(frame, frame);
                }
                if (doNightVision) {
                    // Create a green tint mapping
                    std::vector<cv::Mat> channels;
                    cv::split(frame, channels);
                    channels[0] *= 0.2; // Blue down
                    channels[1] = cv::min(channels[1] * 1.5, 255.0); // Green up
                    channels[2] *= 0.2; // Red down
                    cv::merge(channels, frame);
                }
                if (doCrosshair) {
                    int cx = frame.cols / 2;
                    int cy = frame.rows / 2;
                    cv::line(frame, cv::Point(cx - 20, cy), cv::Point(cx + 20, cy), cv::Scalar(0, 255, 0), 2);
                    cv::line(frame, cv::Point(cx, cy - 20), cv::Point(cx, cy + 20), cv::Scalar(0, 255, 0), 2);
                    cv::putText(frame, "CV Active", cv::Point(10, frame.rows - 20), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
                }
                
                // Encode back to JPEG
                std::vector<uchar> outBuffer;
                cv::imencode(".jpg", frame, outBuffer);
                QByteArray outJpegData(reinterpret_cast<const char*>(outBuffer.data()), outBuffer.size());
                
                // Encode Base64
                QString base64 = "data:image/jpeg;base64," + QString(outJpegData.toBase64());
                
                // Save frame if recording
                if (isRec && !recDir.isEmpty()) {
                    QString framePath = recDir + QString("/frame_%1.jpg").arg(frameNum, 5, 10, QChar('0'));
                    QFile file(framePath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(outJpegData);
                        file.close();
                    }
                }
                
                // Marshall the UI update back to the Main GUI thread safely
                QMetaObject::invokeMethod(this, [this, base64, currentGray]() {
                    m_currentFrameBase64 = base64;
                    if (!currentGray.empty()) {
                        m_prevGrayFrame = currentGray;
                    }
                    emit frameReceived();
                }, Qt::QueuedConnection);
            }
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
