#include "AiVision.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <QByteArray>
#include <QBuffer>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>
#include <cmath>
#include <vector>
#include <algorithm>

static QString colorNameForHue(float h) {
    // h in [0,360)
    if (h < 15 || h >= 345) return "red";
    if (h < 45) return "orange";
    if (h < 70) return "yellow";
    if (h < 160) return "green";
    if (h < 200) return "cyan";
    if (h < 255) return "blue";
    if (h < 290) return "purple";
    if (h < 345) return "pink";
    return "red";
}

static QString lightLevel(float brightness) {
    if (brightness < 28) return "dark";
    if (brightness < 55) return "dim";
    if (brightness < 80) return "bright";
    return "very bright";
}

static QString contrastLevel(float sigma) {
    if (sigma < 18) return "flat/low-contrast";
    if (sigma < 45) return "normal-contrast";
    return "high-contrast";
}

cv::Mat AiVision::toMat(const QImage& img) {
    if (img.isNull() || img.width() <= 0 || img.height() <= 0) return cv::Mat();
    // Convert to a known 32-bit RGB format; on little-endian the raw bytes are
    // [B, G, R, A], which is exactly OpenCV's BGR(A) layout.
    QImage rgb = img.convertToFormat(QImage::Format_RGB32);
    if (rgb.isNull()) return cv::Mat();

    const int w = rgb.width();
    const int h = rgb.height();

    cv::Mat mat(h, w, CV_8UC4);
    for (int y = 0; y < h; ++y) {
        const uchar* src = rgb.constScanLine(y);
        uchar* dst = mat.ptr(y);
        memcpy(dst, src, static_cast<size_t>(w) * 4);
        for (int x = 0; x < w; ++x) dst[x * 4 + 3] = 255;
    }
    return mat;
}

QString AiVision::describeBase64(const QString& base64Jpeg) {
    QByteArray raw = QByteArray::fromBase64(base64Jpeg.toUtf8());
    if (raw.isEmpty()) {
        return QStringLiteral("<no camera feed — frame data is empty>");
    }
    QImage img;
    if (!img.loadFromData(raw)) {
        return QStringLiteral("<camera frame could not be decoded>");
    }
    return describe(img);
}

QString AiVision::describe(const QImage& img) {
    if (img.isNull()) {
        return QStringLiteral("<no camera feed — no frame available>");
    }

    cv::Mat bgr = toMat(img);
    if (bgr.empty()) {
        return QStringLiteral("<camera frame could not be converted for analysis>");
    }

    const int srcW = img.width();
    const int srcH = img.height();

    try {
        int smallW = 96;
        int smallH = std::max(1, (int)std::lround(srcH * (double)smallW / (double)srcW));
        cv::Mat small;
        cv::resize(bgr, small, cv::Size(smallW, smallH));

        cv::Mat gray;
        cv::cvtColor(small, gray, cv::COLOR_BGR2GRAY);
        const int rows = gray.rows, cols = gray.cols;

        // --- brightness / contrast / detail ---
        double sum = 0.0, sumSq = 0.0;
        long count = 0, edgeCount = 0;
        double edgeAcc = 0.0;
        for (int y = 0; y < rows; ++y) {
            const uchar* row = gray.ptr(y);
            for (int x = 0; x < cols; ++x) {
                double v = row[x];
                sum += v;
                sumSq += v * v;
                ++count;
                double dx = 0.0, dy = 0.0;
                if (x > 0 && x < cols - 1) dx = (double)row[x + 1] - (double)row[x - 1];
                if (y > 0 && y < rows - 1) dy = (double)gray.ptr(y + 1)[x] - (double)gray.ptr(y - 1)[x];
                edgeAcc += std::sqrt(dx * dx + dy * dy);
                ++edgeCount;
            }
        }
        float brightness = (float)(sum / (double)count);
        float variance = (float)(sumSq / (double)count - (double)brightness * (double)brightness);
        float contrast = (float)std::sqrt(std::max(0.0, (double)variance));
        float detail = (float)(edgeAcc / (double)std::max(1L, edgeCount));

        // --- dominant colors (saturation weighted) from HSV ---
        cv::Mat hsv;
        cv::cvtColor(small, hsv, cv::COLOR_BGR2HSV);
        const int nbins = 18;
        std::vector<long> hueHist(nbins, 0);
        long totalSat = 0;
        for (int y = 0; y < rows; ++y) {
            const uchar* row = hsv.ptr(y);
            for (int x = 0; x < cols; ++x) {
                uchar hh = row[x * 3];
                uchar ss = row[x * 3 + 1];
                if (ss < 40) continue;
                int idx = (int)(((double)hh / 180.0) * (double)nbins);
                if (idx >= nbins) idx = nbins - 1;
                hueHist[idx]++;
                totalSat++;
            }
        }
        std::vector<int> order(nbins);
        for (int i = 0; i < nbins; ++i) order[i] = i;
        std::sort(order.begin(), order.end(), [&](int a, int b) { return hueHist[a] > hueHist[b]; });

        QStringList colorParts;
        long colorTotal = totalSat > 0 ? totalSat : 1;
        for (int k = 0; k < 3 && k < nbins; ++k) {
            int bin = order[k];
            if (hueHist[bin] <= 0) break;
            int pct = (int)std::lround(((double)hueHist[bin] / (double)colorTotal) * 100.0);
            float hueDeg = (float)(((double)bin + 0.5) / (double)nbins * 360.0);
            QString name = colorNameForHue(hueDeg);
            QString entry = QStringLiteral("%1(%2%)").arg(name).arg(pct);
            if (colorParts.isEmpty() || !colorParts.last().startsWith(name + "(")) {
                colorParts.append(entry);
            }
        }
        long grayPct = 100 - (int)std::lround((double)totalSat / (double)(rows * cols) * 100.0);
        if (grayPct >= 40) colorParts.append(QStringLiteral("gray/white(%1%)").arg(grayPct));
        if (colorParts.isEmpty()) colorParts.append("gray");

        // --- faces (best-effort) ---
        int faces = 0;
        QString cascadePath = QStringLiteral("haarcascade_frontalface_default.xml");
        QFileInfo fi(cascadePath);
        if (fi.exists() && fi.size() > 0) {
            try {
                cv::CascadeClassifier cascade;
                if (cascade.load(cascadePath.toStdString()) && !cascade.empty()) {
                    std::vector<cv::Rect> dets;
                    cascade.detectMultiScale(gray, dets, 1.1, 3, 0, cv::Size(30, 30));
                    faces = (int)dets.size();
                }
            } catch (...) {
                faces = 0;
            }
        }

        // --- composition: center vs overall brightness ---
        int cx0 = cols / 3, cx1 = 2 * cols / 3, cy0 = rows / 3, cy1 = 2 * rows / 3;
        double centerSum = 0.0;
        long centerCount = 0;
        for (int y = cy0; y < cy1; ++y) {
            const uchar* row = gray.ptr(y);
            for (int x = cx0; x < cx1; ++x) {
                centerSum += row[x];
                ++centerCount;
            }
        }
        float centerBright = (float)(centerSum / (double)std::max(1L, centerCount));
        float diff = centerBright - brightness;
        QString composition = QStringLiteral("uniform");
        if (diff > 12) composition = QStringLiteral("bright center (object/light centered)");
        else if (diff < -12) composition = QStringLiteral("dark center (obstacle/shadow centered)");

        QString out = QStringLiteral("FRAME %1x%2 · %3 (%4/100) · %5 (sigma%6) · detail %7\n")
                          .arg(srcW).arg(srcH)
                          .arg(lightLevel(brightness)).arg((int)std::lround(brightness))
                          .arg(contrastLevel(contrast)).arg((int)std::lround(contrast))
                          .arg((int)std::lround(detail));
        out += QStringLiteral("COLORS: %1\n").arg(colorParts.join(", "));
        out += faces > 0 ? QStringLiteral("FACES: %1 person(s) detected\n").arg(faces)
                         : QStringLiteral("FACES: none detected\n");
        out += QStringLiteral("COMPOSITION: %1").arg(composition);
        return out;
    } catch (...) {
        return QStringLiteral("<camera analysis error>");
    }
}