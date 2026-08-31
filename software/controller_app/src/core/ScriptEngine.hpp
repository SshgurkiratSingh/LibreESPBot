#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QRegularExpression>
#include "../network/CommandEmitter.hpp"

class ScriptEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int currentLine READ currentLine NOTIFY currentLineChanged)

public:
    explicit ScriptEngine(CommandEmitter* emitter, QObject *parent = nullptr);

    Q_INVOKABLE void runScript(const QString& scriptText);
    Q_INVOKABLE void stopScript();

    bool isRunning() const { return m_isRunning; }
    int currentLine() const { return m_currentLine; }

signals:
    void isRunningChanged();
    void currentLineChanged();
    void scriptFinished();
    void scriptError(const QString& errorMsg);

private slots:
    void executeNextLine();

private:
    void setRunning(bool running);
    void setCurrentLine(int line);
    bool processCommand(const QString& cmd);

    CommandEmitter* m_emitter;
    QStringList m_lines;
    int m_currentLine;
    bool m_isRunning;
    QTimer m_timer;
};
