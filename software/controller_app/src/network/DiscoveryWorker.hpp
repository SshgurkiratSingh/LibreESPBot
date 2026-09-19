#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QNetworkInterface>
#include "../core/Types.hpp"

class NodeRegistry;

class DiscoveryWorker : public QObject {
    Q_OBJECT

public:
    explicit DiscoveryWorker(NodeRegistry* registry, QObject* parent = nullptr);
    ~DiscoveryWorker();

    void startDiscovery();

    // Manual overrides (still supported for users who know the IP)
    Q_INVOKABLE void setManualIp(const QString& ip);
    Q_INVOKABLE void setManualCameraIp(const QString& ip);

    QString cameraIp() const { return m_cameraIp; }

signals:
    void cameraDiscovered(const QString& ip);

private slots:
    void readPendingDatagrams();
    void readPendingMdnsDatagrams();

private:
    uint16_t calculateCrc16(const uint8_t* data, size_t length);

    NodeRegistry*  m_registry;
    QUdpSocket*    m_socket;
    QUdpSocket*    m_mdnsSocket;
    QString        m_cameraIp;
};
