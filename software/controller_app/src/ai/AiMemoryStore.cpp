#include "AiMemoryStore.hpp"
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QByteArray>
#include <QDebug>

AiMemoryStore::AiMemoryStore(QObject* parent) : QAbstractListModel(parent) {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/LibreESPBot";
    QDir().mkpath(dir);
    m_storeFile = dir + "/memory_notes.json";
    loadFromDisk();
}

int AiMemoryStore::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_notes.size();
}

QVariant AiMemoryStore::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_notes.size()) return QVariant();
    const Note& n = m_notes[index.row()];
    switch (role) {
        case TextRole: return n.text;
        case TimeRole: return n.time;
    }
    return QVariant();
}

QHash<int, QByteArray> AiMemoryStore::roleNames() const {
    return {{TextRole, "text"}, {TimeRole, "time"}};
}

QString AiMemoryStore::remember(const QString& text) {
    QString t = text.trimmed();
    if (t.isEmpty()) return QStringLiteral("<empty note not stored>");
    t = t.left(2000); // safety cap

    Note n;
    n.text = t;
    n.time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    beginInsertRows(QModelIndex(), 0, 0);
    m_notes.prepend(n);
    while (m_notes.size() > 500) m_notes.removeLast(); // bounded
    endInsertRows();
    persist();

    qDebug() << "[AiMemoryStore] remembered:" << t.left(80);
    return QStringLiteral("Stored to memory: \"%1\"").arg(t.left(120));
}

QStringList AiMemoryStore::recall(const QString& query) const {
    QString q = query.trimmed().toLower();
    QStringList out;
    for (const Note& n : m_notes) {
        if (q.isEmpty() || n.text.toLower().contains(q)) {
            out.append(QStringLiteral("[%1] %2").arg(n.time, n.text));
            if (out.size() >= 8) break;
        }
    }
    return out;
}

QStringList AiMemoryStore::allNotes() const {
    QStringList out;
    for (const Note& n : m_notes) out.append(QStringLiteral("[%1] %2").arg(n.time, n.text));
    return out;
}

void AiMemoryStore::clear() {
    if (m_notes.isEmpty()) return;
    beginResetModel();
    m_notes.clear();
    endResetModel();
    persist();
}

void AiMemoryStore::loadFromDisk() {
    QFile f(m_storeFile);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return;
    QByteArray data = f.readAll();
    f.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) return;

    beginResetModel();
    m_notes.clear();
    QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        Note n;
        n.text = o["text"].toString();
        n.time = o["time"].toString();
        if (!n.text.isEmpty()) m_notes.append(n);
    }
    endResetModel();
}

void AiMemoryStore::persist() {
    QJsonArray arr;
    for (const Note& n : m_notes) {
        QJsonObject o;
        o["text"] = n.text;
        o["time"] = n.time;
        arr.append(o);
    }
    QFile f(m_storeFile);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        f.close();
    } else {
        qWarning() << "[AiMemoryStore] Failed to persist memory to:" << m_storeFile;
    }
}