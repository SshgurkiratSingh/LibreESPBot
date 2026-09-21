#pragma once

#include <QString>
#include <QImage>
#include <opencv2/core.hpp>

// AiVision: local, text-based scene analysis for the rover AI.
//
// DeepSeek-flash accepts real images, but many providers/models are text-only
// and reject image_url parts. AiVision therefore gives every model a second,
// provider-agnostic "sight" channel: it analyzes frames here with OpenCV and
// returns a concise, factual text description of what the camera sees. This
// description is handed to text-only models, and complements the real pixels
// that are also embedded for vision-capable models (see AiAgentEngine).
class AiVision {
public:
    // Produce a compact textual description of `img` (dominant colors,
    // lighting, detail, faces, composition). Returns a short multi-line string
    // (safe/meaningful even for a null image).
    static QString describe(const QImage& img);

    // Convenience: decode a base64 JPEG/PNG payload and describe it.
    static QString describeBase64(const QString& base64Jpeg);

    // Best-effort QImage -> OpenCV BGR(A) matrix. Empty matrix on failure.
    static cv::Mat toMat(const QImage& img);
};