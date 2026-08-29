#pragma once

#include <QObject>
#include <QUdpSocket>

class DiscoveryWorker : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString roverIp READ roverIp NOTIFY roverDiscovered)
    Q_PROPERTY(QString hardwareProfile READ hardwareProfile NOTIFY roverDiscovered)

public:
    explicit DiscoveryWorker(QObject *parent = nullptr);
    ~DiscoveryWorker();

    void startDiscovery();
    
public slots:
    void setManualIp(const QString& ip);
    
    QString roverIp() const { return m_roverIp; }
    QString hardwareProfile() const { return m_hardwareProfile; }

signals:
    void roverDiscovered(const QString& ip, const QString& profile);

private slots:
    void readPendingDatagrams();

private:
    QUdpSocket *m_socket;
    QString m_roverIp;
    QString m_hardwareProfile;
};
