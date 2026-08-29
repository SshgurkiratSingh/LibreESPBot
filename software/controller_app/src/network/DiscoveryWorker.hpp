#pragma once

#include <QObject>
#include <QUdpSocket>

class DiscoveryWorker : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString roverIp READ roverIp NOTIFY roverDiscovered)
    Q_PROPERTY(QString hardwareProfile READ hardwareProfile NOTIFY roverDiscovered)
    Q_PROPERTY(QString cameraIp READ cameraIp NOTIFY cameraDiscovered)

public:
    explicit DiscoveryWorker(QObject *parent = nullptr);
    ~DiscoveryWorker();

    void startDiscovery();
    
public slots:
    void setManualIp(const QString& ip);
    void setManualCameraIp(const QString& ip);
    
    QString roverIp() const { return m_roverIp; }
    QString hardwareProfile() const { return m_hardwareProfile; }
    QString cameraIp() const { return m_cameraIp; }

signals:
    void roverDiscovered(const QString& ip, const QString& profile);
    void cameraDiscovered(const QString& ip);

private slots:
    void readPendingDatagrams();

private:
    QUdpSocket *m_socket;
    QString m_roverIp;
    QString m_hardwareProfile;
    QString m_cameraIp;
};
