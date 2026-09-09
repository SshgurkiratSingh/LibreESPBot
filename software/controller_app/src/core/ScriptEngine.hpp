#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QRegularExpression>
#include <QElapsedTimer>
#include "../network/CommandEmitter.hpp"
#include "../network/TelemetryClient.hpp"

class ScriptEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(int currentLine READ currentLine NOTIFY currentLineChanged)

public:
    explicit ScriptEngine(CommandEmitter* emitter, TelemetryClient* telemetry = nullptr, QObject *parent = nullptr);

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
    void onTurnToTick();      // Called by m_turnTimer to poll compass and adjust

private:
    void setRunning(bool running);
    void setCurrentLine(int line);
    bool processCommand(const QString& cmd);

    // Utility: shortest angular distance from current heading to target (-180..+180)
    static float angleDiff(float from, float to);

    CommandEmitter*   m_emitter;
    TelemetryClient*  m_telemetry;
    QStringList       m_lines;
    int               m_currentLine;
    bool              m_isRunning;
    QTimer            m_timer;       // General wait / delay timer

    // --- turn_to state ---
    QTimer            m_turnTimer;   // Polls telemetry for arrival
    float             m_turnTarget;  // Target compass heading (0-359)
    static constexpr float kTolerance   = 5.0f;   // degrees
    static constexpr int   kTickMs      = 100;    // ms per feedback tick (10 Hz)
};
