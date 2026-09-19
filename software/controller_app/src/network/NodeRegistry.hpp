#pragma once
#include <QAbstractListModel>
#include <QList>
#include <QTimer>
#include "RoverNode.hpp"

class NodeRegistry : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY layoutChanged)
    Q_PROPERTY(int activeIndex READ activeIndex NOTIFY activeIndexChanged)
    Q_PROPERTY(RoverNode* activeNode READ activeNode NOTIFY activeIndexChanged)

public:
    enum Roles {
        IpRole = Qt::UserRole + 1,
        BoardNameRole,
        FwVersionRole,
        BatteryRole,
        ConnectedRole,
        RttRole,
        StatusRole,
        BoardTypeRole,
        VersionMismatchRole,
        ObjectRole  // returns the RoverNode* QObject itself
    };
    Q_ENUM(Roles)

    explicit NodeRegistry(QObject* parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void selectNode(int index);
    Q_INVOKABLE void addManualNode(const QString& ip);

    int activeIndex() const { return m_activeIndex; }
    RoverNode* activeNode() const;

    // Returns existing node for ip, or creates a new one and appends it
    RoverNode* nodeForIp(const QString& ip);

signals:
    void activeIndexChanged();
    void activeNodeChanged(RoverNode* node);
    void nodeDiscovered(const QString& ip);

private slots:
    void pruneStaleNodes();
    void onNodeUpdated();

private:
    QList<RoverNode*> m_nodes;
    int m_activeIndex = -1;
    QTimer* m_pruneTimer;
    QTimer* m_healthTimer;

    static constexpr qint64 PRUNE_TIMEOUT_MS = 30000;
};
