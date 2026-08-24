#include "RadarPointCloud.hpp"
#include <cmath>
#include <QDateTime>
#include <QVariant>

#define MAX_POINTS 360
#define POINT_LIFETIME_MS 5000

RadarPointCloud::RadarPointCloud(QObject *parent) : QObject(parent) {
    m_pointBuffer.reserve(MAX_POINTS);
}

void RadarPointCloud::addPolarPoint(int distanceMm, float angleDeg) {
    if (distanceMm == 0 || distanceMm > 2000) return; // Out of reliable range

    // Convert Polar to Cartesian
    // angleDeg: 0 is forward, positive is right, negative is left
    float angleRad = angleDeg * M_PI / 180.0f;
    float x = distanceMm * std::sin(angleRad);
    float y = distanceMm * std::cos(angleRad);
    
    QMutexLocker locker(&m_mutex);
    long now = QDateTime::currentMSecsSinceEpoch();
    
    // Add new point
    m_pointBuffer.append({x, y, now});
    
    // Remove expired points
    while (!m_pointBuffer.isEmpty() && (now - m_pointBuffer.first().timestamp > POINT_LIFETIME_MS)) {
        m_pointBuffer.removeFirst();
    }
    
    // Cap maximum points
    while (m_pointBuffer.size() > MAX_POINTS) {
        m_pointBuffer.removeFirst();
    }
    
    locker.unlock();
    emit pointsUpdated();
}

QVariantList RadarPointCloud::points() const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    QVariantList list;
    for (const auto& p : m_pointBuffer) {
        QVariantMap map;
        map["x"] = p.x;
        map["y"] = p.y;
        list.append(map);
    }
    return list;
}
