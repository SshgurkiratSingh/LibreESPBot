#pragma once

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QPointF>
#include <QMutex>

class RadarPointCloud : public QObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList points READ points NOTIFY pointsUpdated)

public:
    explicit RadarPointCloud(QObject *parent = nullptr);
    
    Q_INVOKABLE void addPolarPoint(int distanceMm, float angleDeg);
    
    QVariantList points() const;

signals:
    void pointsUpdated();

private:
    struct CartesianPoint {
        float x;
        float y;
        long timestamp;
    };
    
    QVector<CartesianPoint> m_pointBuffer;
    QMutex m_mutex;
};
