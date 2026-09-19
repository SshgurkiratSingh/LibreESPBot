#include "NodeRegistry.hpp"
#include <QDateTime>

NodeRegistry::NodeRegistry(QObject* parent)
    : QAbstractListModel(parent) {
    
    m_pruneTimer = new QTimer(this);
    connect(m_pruneTimer, &QTimer::timeout, this, &NodeRegistry::pruneStaleNodes);
    m_pruneTimer->start(10000); // every 10s

    m_healthTimer = new QTimer(this);
    // Use lambda or direct connection. checkHealth isn't a slot but we can use lambda.
    connect(m_healthTimer, &QTimer::timeout, this, [this]() {
        for (RoverNode* node : m_nodes) {
            node->checkHealth();
        }
    });
    m_healthTimer->start(500); // every 500ms
}

int NodeRegistry::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_nodes.size();
}

QVariant NodeRegistry::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_nodes.size()) return QVariant();

    RoverNode* node = m_nodes[index.row()];
    switch (role) {
        case IpRole: return node->ip();
        case BoardNameRole: return node->boardName();
        case FwVersionRole: return node->fwVersion();
        case BatteryRole: return node->battery();
        case ConnectedRole: return node->connected();
        case RttRole: return node->rttMs();
        case StatusRole: return node->statusText();
        case BoardTypeRole: return node->boardType();
        case VersionMismatchRole: return node->versionMismatch();
        case ObjectRole: return QVariant::fromValue(node);
        default: return QVariant();
    }
}

QHash<int, QByteArray> NodeRegistry::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IpRole] = "ip";
    roles[BoardNameRole] = "boardName";
    roles[FwVersionRole] = "fwVersion";
    roles[BatteryRole] = "battery";
    roles[ConnectedRole] = "connected";
    roles[RttRole] = "rttMs";
    roles[StatusRole] = "statusText";
    roles[BoardTypeRole] = "boardType";
    roles[VersionMismatchRole] = "versionMismatch";
    roles[ObjectRole] = "nodeData";
    return roles;
}

void NodeRegistry::selectNode(int index) {
    if (index < 0 || index >= m_nodes.size()) return;
    
    m_activeIndex = index;
    emit activeIndexChanged();
    emit activeNodeChanged(m_nodes[index]);
}

void NodeRegistry::addManualNode(const QString& ip) {
    RoverNode* node = nodeForIp(ip);
    int index = m_nodes.indexOf(node);
    if (index >= 0) {
        selectNode(index);
    }
}

RoverNode* NodeRegistry::activeNode() const {
    if (m_activeIndex >= 0 && m_activeIndex < m_nodes.size()) {
        return m_nodes[m_activeIndex];
    }
    return nullptr;
}

RoverNode* NodeRegistry::nodeForIp(const QString& ip) {
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i]->ip() == ip) {
            return m_nodes[i];
        }
    }

    int newIndex = m_nodes.size();
    beginInsertRows(QModelIndex(), newIndex, newIndex);
    RoverNode* newNode = new RoverNode(ip, this);
    connect(newNode, &RoverNode::updated, this, &NodeRegistry::onNodeUpdated);
    m_nodes.append(newNode);
    endInsertRows();
    emit layoutChanged();

    if (m_activeIndex == -1) {
        m_activeIndex = 0;
        emit activeIndexChanged();
        emit activeNodeChanged(newNode);
    }

    emit nodeDiscovered(ip);
    return newNode;
}

void NodeRegistry::pruneStaleNodes() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    for (int i = m_nodes.size() - 1; i >= 0; --i) {
        if ((now - m_nodes[i]->lastSeenMs()) > PRUNE_TIMEOUT_MS) {
            beginRemoveRows(QModelIndex(), i, i);
            RoverNode* node = m_nodes.takeAt(i);
            endRemoveRows();
            
            node->deleteLater();
            
            if (m_activeIndex == i) {
                m_activeIndex = -1;
                emit activeIndexChanged();
                emit activeNodeChanged(nullptr);
            } else if (m_activeIndex > i) {
                m_activeIndex--;
                emit activeIndexChanged();
            }
        }
    }
    emit layoutChanged();
}

void NodeRegistry::onNodeUpdated() {
    RoverNode* senderNode = qobject_cast<RoverNode*>(sender());
    if (!senderNode) return;

    int row = m_nodes.indexOf(senderNode);
    if (row >= 0) {
        QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx);
    }
}
