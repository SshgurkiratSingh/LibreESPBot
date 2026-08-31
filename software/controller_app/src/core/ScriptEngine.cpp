#include "ScriptEngine.hpp"
#include <QDebug>

ScriptEngine::ScriptEngine(CommandEmitter* emitter, QObject *parent)
    : QObject(parent), m_emitter(emitter), m_currentLine(-1), m_isRunning(false)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &ScriptEngine::executeNextLine);
}

void ScriptEngine::runScript(const QString& scriptText)
{
    if (m_isRunning) {
        stopScript();
    }

    m_lines = scriptText.split('\n');
    setCurrentLine(-1);
    setRunning(true);
    
    // Start immediately
    QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
}

void ScriptEngine::stopScript()
{
    m_timer.stop();
    setRunning(false);
    
    // Safety fallback
    if (m_emitter) {
        m_emitter->updateThrottle(0);
        m_emitter->updateSteering(0);
    }
}

void ScriptEngine::executeNextLine()
{
    if (!m_isRunning) return;

    setCurrentLine(m_currentLine + 1);

    if (m_currentLine >= m_lines.size()) {
        stopScript();
        emit scriptFinished();
        return;
    }

    QString line = m_lines.at(m_currentLine).trimmed();
    
    // Skip empty lines or comments
    if (line.isEmpty() || line.startsWith("//") || line.startsWith("#")) {
        // Execute next immediately
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
        return;
    }

    bool success = processCommand(line);
    if (!success) {
        stopScript();
        emit scriptError(QString("Syntax Error on line %1: %2").arg(m_currentLine + 1).arg(line));
        return;
    }
}

bool ScriptEngine::processCommand(const QString& cmd)
{
    // Basic regex: commandName(argument)
    QRegularExpression re("^([a-zA-Z_]+)\\(([-0-9]*)\\)$");
    QRegularExpressionMatch match = re.match(cmd);
    
    if (!match.hasMatch()) {
        return false;
    }

    QString command = match.captured(1);
    bool ok = false;
    int arg = match.captured(2).toInt(&ok);
    if (match.captured(2).isEmpty()) {
        arg = 0; // Default argument if empty
        ok = true;
    }

    if (!ok) return false;

    if (command == "throttle" || command == "forward") {
        // Scale 0-100 to 0-1023
        int16_t mapped = (arg * 1023) / 100;
        if (m_emitter) m_emitter->updateThrottle(mapped);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
    } else if (command == "reverse") {
        int16_t mapped = (-arg * 1023) / 100;
        if (m_emitter) m_emitter->updateThrottle(mapped);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
    } else if (command == "steer") {
        int16_t mapped = (arg * 1023) / 100;
        if (m_emitter) m_emitter->updateSteering(mapped);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
    } else if (command == "headlight") {
        if (m_emitter) m_emitter->setHeadlightMode(arg);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
    } else if (command == "stop") {
        if (m_emitter) m_emitter->updateThrottle(0);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
    } else if (command == "wait") {
        if (arg > 0) {
            m_timer.start(arg);
        } else {
            QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
        }
    } else {
        return false; // Unknown command
    }

    return true;
}

void ScriptEngine::setRunning(bool running)
{
    if (m_isRunning != running) {
        m_isRunning = running;
        emit isRunningChanged();
    }
}

void ScriptEngine::setCurrentLine(int line)
{
    if (m_currentLine != line) {
        m_currentLine = line;
        emit currentLineChanged();
    }
}
