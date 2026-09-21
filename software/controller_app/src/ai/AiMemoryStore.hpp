#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QAbstractListModel>

// Persistent text memory for the rover AI. The agent writes facts/notes via the
// `remember` tool and retrieves them via `recall_memory`. Notes survive app
// restarts (persisted to a small JSON file next to the saved images) and are
// also exposed as a QML list model so the UI can show them.
class AiMemoryStore : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { TextRole = Qt::UserRole + 1, TimeRole };

    explicit AiMemoryStore(QObject* parent = nullptr);

    // QAbstractListModel overrides
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Agent tool API
    QString remember(const QString& text);           // stores + persists, returns confirmation text
    QStringList recall(const QString& query) const;  // case-insensitive substring search (newest first)
    QStringList allNotes() const;
    Q_INVOKABLE void clear();

private:
    void loadFromDisk();
    void persist();

    struct Note {
        QString text;
        QString time;
    };
    QList<Note> m_notes;
    QString m_storeFile;
};