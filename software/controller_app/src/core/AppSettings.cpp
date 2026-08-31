#include "AppSettings.hpp"

AppSettings::AppSettings(QObject *parent) 
    : QObject(parent), 
      m_settings(QSettings::IniFormat, QSettings::UserScope, "LibreESP", "LibreESPBot") 
{
    // Load existing settings or set defaults
    m_radarPointLifetimeMs = m_settings.value("Radar/PointLifetimeMs", 5000).toInt();
    m_sensorBaseAngleDeg = m_settings.value("Radar/SensorBaseAngleDeg", 0).toInt();
    m_invertTof = m_settings.value("Radar/InvertTof", false).toBool();

    m_display3dKinematics = m_settings.value("Display/3dKinematics", false).toBool();
    m_displayHudDebug = m_settings.value("Display/HudDebug", true).toBool();
    m_maxThrottleLimit = m_settings.value("Control/MaxThrottleLimit", 100).toInt();
    m_steeringSensitivity = m_settings.value("Control/SteeringSensitivity", 1.0f).toFloat();
    m_lowBatteryWarningVolts = m_settings.value("Safety/LowBatteryWarningVolts", 7.0f).toFloat();
    m_voltageScaleMultiplier = m_settings.value("Hardware/VoltageScaleMultiplier", 1.0f).toFloat();
    m_pitchOffset = m_settings.value("Calibration/PitchOffset", 0.0f).toFloat();
    m_rollOffset = m_settings.value("Calibration/RollOffset", 0.0f).toFloat();
    m_motorRpm = m_settings.value("Hardware/MotorRpm", 300).toInt();
    m_wheelSizeMm = m_settings.value("Hardware/WheelSizeMm", 60).toInt();
    m_hudOpacity = m_settings.value("HUD/Opacity", 0.8f).toFloat();
    m_hudColor = m_settings.value("HUD/Color", "#00E5FF").toString();
}

int AppSettings::radarPointLifetimeMs() const {
    return m_radarPointLifetimeMs;
}

void AppSettings::setRadarPointLifetimeMs(int ms) {
    if (m_radarPointLifetimeMs != ms) {
        m_radarPointLifetimeMs = ms;
        m_settings.setValue("Radar/PointLifetimeMs", ms);
        emit radarPointLifetimeMsChanged();
    }
}

int AppSettings::sensorBaseAngleDeg() const {
    return m_sensorBaseAngleDeg;
}

void AppSettings::setSensorBaseAngleDeg(int offset) {
    if (m_sensorBaseAngleDeg != offset) {
        m_sensorBaseAngleDeg = offset;
        m_settings.setValue("Radar/SensorBaseAngleDeg", offset);
        emit sensorBaseAngleDegChanged();
    }
}

bool AppSettings::invertTof() const {
    return m_invertTof;
}

void AppSettings::setInvertTof(bool invert) {
    if (m_invertTof != invert) {
        m_invertTof = invert;
        m_settings.setValue("Radar/InvertTof", invert);
        emit invertTofChanged();
    }
}

bool AppSettings::display3dKinematics() const {
    return m_display3dKinematics;
}

void AppSettings::setDisplay3dKinematics(bool display) {
    if (m_display3dKinematics != display) {
        m_display3dKinematics = display;
        m_settings.setValue("Display/3dKinematics", display);
        emit display3dKinematicsChanged();
    }
}

bool AppSettings::displayHudDebug() const {
    return m_displayHudDebug;
}

void AppSettings::setDisplayHudDebug(bool display) {
    if (m_displayHudDebug != display) {
        m_displayHudDebug = display;
        m_settings.setValue("Display/HudDebug", display);
        emit displayHudDebugChanged();
    }
}

int AppSettings::maxThrottleLimit() const {
    return m_maxThrottleLimit;
}

void AppSettings::setMaxThrottleLimit(int limit) {
    if (m_maxThrottleLimit != limit) {
        m_maxThrottleLimit = limit;
        m_settings.setValue("Control/MaxThrottleLimit", limit);
        emit maxThrottleLimitChanged();
    }
}

float AppSettings::steeringSensitivity() const {
    return m_steeringSensitivity;
}

void AppSettings::setSteeringSensitivity(float sensitivity) {
    if (m_steeringSensitivity != sensitivity) {
        m_steeringSensitivity = sensitivity;
        m_settings.setValue("Control/SteeringSensitivity", sensitivity);
        emit steeringSensitivityChanged();
    }
}

float AppSettings::lowBatteryWarningVolts() const {
    return m_lowBatteryWarningVolts;
}

void AppSettings::setLowBatteryWarningVolts(float volts) {
    if (m_lowBatteryWarningVolts != volts) {
        m_lowBatteryWarningVolts = volts;
        m_settings.setValue("Safety/LowBatteryWarningVolts", volts);
        emit lowBatteryWarningVoltsChanged();
    }
}

float AppSettings::voltageScaleMultiplier() const {
    return m_voltageScaleMultiplier;
}

void AppSettings::setVoltageScaleMultiplier(float multiplier) {
    if (qFuzzyCompare(m_voltageScaleMultiplier, multiplier)) return;
    m_voltageScaleMultiplier = multiplier;
    m_settings.setValue("Hardware/VoltageScaleMultiplier", m_voltageScaleMultiplier);
    emit voltageScaleMultiplierChanged();
}

void AppSettings::setPitchOffset(float offset) {
    if (qFuzzyCompare(m_pitchOffset, offset)) return;
    m_pitchOffset = offset;
    m_settings.setValue("Calibration/PitchOffset", m_pitchOffset);
    emit pitchOffsetChanged();
}

void AppSettings::setRollOffset(float offset) {
    if (qFuzzyCompare(m_rollOffset, offset)) return;
    m_rollOffset = offset;
    m_settings.setValue("Calibration/RollOffset", m_rollOffset);
    emit rollOffsetChanged();
}

void AppSettings::calibrateLevel(float currentPitch, float currentRoll) {
    setPitchOffset(currentPitch);
    setRollOffset(currentRoll);
}

void AppSettings::resetLevelCalibration() {
    setPitchOffset(0.0f);
    setRollOffset(0.0f);
}

void AppSettings::setMotorRpm(int rpm) {
    if (m_motorRpm == rpm) return;
    m_motorRpm = rpm;
    m_settings.setValue("Hardware/MotorRpm", m_motorRpm);
    emit motorRpmChanged();
}

void AppSettings::setWheelSizeMm(int size) {
    if (m_wheelSizeMm == size) return;
    m_wheelSizeMm = size;
    m_settings.setValue("Hardware/WheelSizeMm", m_wheelSizeMm);
    emit wheelSizeMmChanged();
}

void AppSettings::setHudOpacity(float opacity) {
    if (qFuzzyCompare(m_hudOpacity, opacity)) return;
    m_hudOpacity = opacity;
    m_settings.setValue("HUD/Opacity", m_hudOpacity);
    emit hudOpacityChanged();
}

void AppSettings::setHudColor(const QString& color) {
    if (m_hudColor == color) return;
    m_hudColor = color;
    m_settings.setValue("HUD/Color", m_hudColor);
    emit hudColorChanged();
}
