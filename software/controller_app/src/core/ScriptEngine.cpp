#include "ScriptEngine.hpp"
#include <QDebug>
#include <cmath>

// ---------------------------------------------------------------------------
//  Constructor
// ---------------------------------------------------------------------------
ScriptEngine::ScriptEngine(CommandEmitter* emitter, TelemetryClient* telemetry, QObject *parent)
    : QObject(parent)
    , m_emitter(emitter)
    , m_telemetry(telemetry)
    , m_currentLine(-1)
    , m_isRunning(false)
    , m_turnTarget(0.0f)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &ScriptEngine::executeNextLine);

    m_turnTimer.setInterval(kTickMs);
    connect(&m_turnTimer, &QTimer::timeout, this, &ScriptEngine::onTurnToTick);
}

// ---------------------------------------------------------------------------
//  Public API
// ---------------------------------------------------------------------------
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
    m_turnTimer.stop();
    setRunning(false);

    // Safety fallback
    if (m_emitter) {
        m_emitter->setAutoTurn(false, 0); // CRITICAL: Disable firmware auto-turn
        m_emitter->updateThrottle(0);
        m_emitter->updateSteering(0);
    }
}

// ---------------------------------------------------------------------------
//  Private: step sequencer
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
//  Private: command dispatcher
// ---------------------------------------------------------------------------
bool ScriptEngine::processCommand(const QString& cmd)
{
    // Regex: commandName(argument)  — argument may be empty, int, or float
    QRegularExpression re(R"(^([a-zA-Z_]+)\(([-0-9.]*)?\)$)");
    QRegularExpressionMatch match = re.match(cmd);

    if (!match.hasMatch()) {
        return false;
    }

    QString command   = match.captured(1);
    QString argStr    = match.captured(2);

    bool okInt   = false;
    bool okFloat = false;
    int   argInt   = argStr.toInt(&okInt);
    float argFloat = argStr.toFloat(&okFloat);

    if (argStr.isEmpty()) {
        argInt   = 0;
        argFloat = 0.0f;
        okInt    = true;
        okFloat  = true;
    }

    // ------------------------------------------------------------------
    //  Existing commands (unchanged behaviour)
    // ------------------------------------------------------------------
    if (command == "throttle" || command == "forward") {
        if (!okInt) return false;
        int mapped = (argInt * 1023) / 100;
        if (m_emitter) m_emitter->updateThrottle(mapped);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);

    } else if (command == "reverse") {
        if (!okInt) return false;
        int mapped = (-argInt * 1023) / 100;
        if (m_emitter) m_emitter->updateThrottle(mapped);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);

    } else if (command == "steer") {
        if (!okInt) return false;
        int mapped = (argInt * 1023) / 100;
        if (m_emitter) m_emitter->updateSteering(mapped);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);

    } else if (command == "headlight") {
        if (!okInt) return false;
        if (m_emitter) m_emitter->setHeadlightMode(argInt);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);

    } else if (command == "stop") {
        if (m_emitter) m_emitter->updateThrottle(0);
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);

    } else if (command == "wait") {
        if (!okInt) return false;
        if (argInt > 0) {
            m_timer.start(argInt);
        } else {
            QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
        }

    // ------------------------------------------------------------------
    //  NEW: turn_to(degrees)  — closed-loop compass turn
    //
    //  Usage:  turn_to(180)   → turn to magnetic South
    //          turn_to(0)     → turn to magnetic North
    //          turn_to(270)   → turn West
    //
    //  The rover steers in the shortest direction, monitors compass yaw
    //  for actual movement, ramps up speed if it detects it's stuck,
    //  and stops once within ±5° of the target.
    // ------------------------------------------------------------------
    } else if (command == "turn_to") {
        if (!okFloat) return false;

        // Normalise target to [0, 360)
        float target = fmodf(argFloat, 360.0f);
        if (target < 0) target += 360.0f;

        if (!m_telemetry) {
            // No telemetry — skip command silently
            qWarning() << "ScriptEngine: turn_to called but no TelemetryClient available";
            QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
            return true;
        }

        // Already within tolerance? Skip immediately.
        float currentHeading = m_telemetry->headingCompassDeg();
        if (qAbs(angleDiff(currentHeading, target)) <= kTolerance) {
            if (m_emitter) m_emitter->updateSteering(0);
            QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
            return true;
        }

        // Initialise state machine
        m_turnTarget  = target;

        if (m_emitter) {
            m_emitter->setAutoTurn(true, m_turnTarget);
        }

        // Start polling timer
        m_turnTimer.start();

    } else {
        return false; // Unknown command
    }

    return true;
}

// ---------------------------------------------------------------------------
//  Private slot: Polling timer to check if firmware auto-turn is complete
// ---------------------------------------------------------------------------
void ScriptEngine::onTurnToTick()
{
    if (!m_isRunning || !m_emitter || !m_telemetry) {
        m_turnTimer.stop();
        return;
    }

    float heading = m_telemetry->headingCompassDeg();
    float diff    = angleDiff(heading, m_turnTarget);

    // --- Arrived? --------------------------------------------------------
    if (qAbs(diff) <= kTolerance) {
        m_turnTimer.stop();
        m_emitter->setAutoTurn(false, 0); // Disable auto-turn on firmware
        
        // Advance script to next line
        QMetaObject::invokeMethod(this, "executeNextLine", Qt::QueuedConnection);
    }
}

// ---------------------------------------------------------------------------
//  Utility: shortest signed angular distance from → to, result in (-180, 180]
// ---------------------------------------------------------------------------
float ScriptEngine::angleDiff(float from, float to)
{
    float d = fmodf(to - from, 360.0f);
    if (d > 180.0f)  d -= 360.0f;
    if (d <= -180.0f) d += 360.0f;
    return d;
}

// ---------------------------------------------------------------------------
//  Internal setters
// ---------------------------------------------------------------------------
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
