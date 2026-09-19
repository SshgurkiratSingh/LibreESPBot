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
    void onTurnPulseDone();    // Called after a short drive burst — stop & measure compass
    void onTurnSettleDone();   // Called after settle pause — decide next pulse

private:
    void setRunning(bool running);
    void setCurrentLine(int line);
    bool processCommand(const QString& cmd);
    void startNextTurnPulse(); // Core pulsed turn helper

    // Utility: shortest angular distance from current heading to target (-180..+180)
    static float angleDiff(float from, float to);

    CommandEmitter*   m_emitter;
    TelemetryClient*  m_telemetry;
    QStringList       m_lines;
    int               m_currentLine;
    bool              m_isRunning;
    QTimer            m_timer;        // General wait / delay timer

    // -----------------------------------------------------------------------
    // turn_to state — pulsed approach:
    //   1. Issue a short burst (kPulseMs) at kTurnSpeed in the correct direction
    //   2. Stop both motors
    //   3. Wait kSettleMs for the platform to stop and compass to stabilise
    //   4. Read heading — if within tolerance, done. Else repeat from 1.
    // -----------------------------------------------------------------------
    QTimer   m_turnPulseTimer;   // Fires after drive burst — triggers stop
    QTimer   m_turnSettleTimer;  // Fires after settle pause — triggers compass check
    float    m_turnTarget;       // Target compass heading (0-359)
    int      m_turnTimeoutMs;    // Safety: abort after this many ms of trying
    QElapsedTimer m_turnElapsed; // Tracks total elapsed time for the timeout

    // Tuning constants
    static constexpr float kTolerance     =  5.0f;  // degrees — arrival band
    static constexpr int   kPulseMs       =  350;   // ms of actual turning per burst (increased)
    static constexpr int   kSettleMs      =  500;   // ms to wait after stopping for compass to calm (increased)
    static constexpr int   kMinTurnSpeed  =  358;   // PWM min 35% of 1023
    static constexpr int   kMaxTurnSpeed  =  512;   // PWM max 50% of 1023
    static constexpr int   kDefaultTurnTimeoutMs = 15000; // give up after 15 s
};
