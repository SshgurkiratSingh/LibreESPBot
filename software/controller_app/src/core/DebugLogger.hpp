#pragma once
#include <QObject>
#include <QStringList>
#include <QMutex>

class DebugLogger : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList logMessages READ logMessages NOTIFY logMessagesChanged)
public:
    static DebugLogger* instance();
    QStringList logMessages() const;
    Q_INVOKABLE void clear();
    void addMessage(const QString& msg);

signals:
    void logMessagesChanged();

private:
    DebugLogger(QObject* parent = nullptr) : QObject(parent) {}
    QStringList m_messages;
    QMutex m_mutex;
};

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
