#include "DebugLogger.hpp"

DebugLogger* DebugLogger::instance() {
    static DebugLogger inst;
    return &inst;
}

QStringList DebugLogger::logMessages() const {
    return m_messages;
}

void DebugLogger::clear() {
    QMutexLocker locker(&m_mutex);
    m_messages.clear();
    emit logMessagesChanged();
}

void DebugLogger::addMessage(const QString& msg) {
    QMutexLocker locker(&m_mutex);
    m_messages.append(msg);
    if (m_messages.size() > 100) {
        m_messages.removeFirst();
    }
    emit logMessagesChanged();
}

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QString txt;
    switch (type) {
        case QtDebugMsg:    txt = QString("[Debug] %1").arg(msg); break;
        case QtWarningMsg:  txt = QString("[Warning] %1").arg(msg); break;
        case QtCriticalMsg: txt = QString("[Critical] %1").arg(msg); break;
        case QtFatalMsg:    txt = QString("[Fatal] %1").arg(msg); break;
        case QtInfoMsg:     txt = QString("[Info] %1").arg(msg); break;
    }
    DebugLogger::instance()->addMessage(txt);
}
