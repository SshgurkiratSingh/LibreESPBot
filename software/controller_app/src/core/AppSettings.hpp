#pragma once

#include <QObject>
#include <QSettings>
#include <QCoreApplication>

class AppSettings : public QObject {
    Q_OBJECT

    Q_PROPERTY(int radarPointLifetimeMs READ radarPointLifetimeMs WRITE setRadarPointLifetimeMs NOTIFY radarPointLifetimeMsChanged)
    Q_PROPERTY(int sensorBaseAngleDeg READ sensorBaseAngleDeg WRITE setSensorBaseAngleDeg NOTIFY sensorBaseAngleDegChanged)
    Q_PROPERTY(bool invertTof READ invertTof WRITE setInvertTof NOTIFY invertTofChanged)
    Q_PROPERTY(int radarSweepSpeed READ radarSweepSpeed WRITE setRadarSweepSpeed NOTIFY radarSweepSpeedChanged)

    Q_PROPERTY(bool display3dKinematics READ display3dKinematics WRITE setDisplay3dKinematics NOTIFY display3dKinematicsChanged)
    Q_PROPERTY(bool displayHudDebug READ displayHudDebug WRITE setDisplayHudDebug NOTIFY displayHudDebugChanged)
    Q_PROPERTY(int maxThrottleLimit READ maxThrottleLimit WRITE setMaxThrottleLimit NOTIFY maxThrottleLimitChanged)
    Q_PROPERTY(float steeringSensitivity READ steeringSensitivity WRITE setSteeringSensitivity NOTIFY steeringSensitivityChanged)
    Q_PROPERTY(float lowBatteryWarningVolts READ lowBatteryWarningVolts WRITE setLowBatteryWarningVolts NOTIFY lowBatteryWarningVoltsChanged)
    Q_PROPERTY(float voltageScaleMultiplier READ voltageScaleMultiplier WRITE setVoltageScaleMultiplier NOTIFY voltageScaleMultiplierChanged)

    Q_PROPERTY(int motorRpm READ motorRpm WRITE setMotorRpm NOTIFY motorRpmChanged)
    Q_PROPERTY(int wheelSizeMm READ wheelSizeMm WRITE setWheelSizeMm NOTIFY wheelSizeMmChanged)
    Q_PROPERTY(float hudOpacity READ hudOpacity WRITE setHudOpacity NOTIFY hudOpacityChanged)
    Q_PROPERTY(QString hudColor READ hudColor WRITE setHudColor NOTIFY hudColorChanged)

public:
    Q_PROPERTY(float pitchOffset READ pitchOffset WRITE setPitchOffset NOTIFY pitchOffsetChanged)
    Q_PROPERTY(float rollOffset READ rollOffset WRITE setRollOffset NOTIFY rollOffsetChanged)

    explicit AppSettings(QObject *parent = nullptr);

    int radarPointLifetimeMs() const;
    void setRadarPointLifetimeMs(int ms);

    int sensorBaseAngleDeg() const;
    void setSensorBaseAngleDeg(int offset);

    bool invertTof() const;
    void setInvertTof(bool invert);

    int radarSweepSpeed() const;
    void setRadarSweepSpeed(int speed);

    bool display3dKinematics() const;
    void setDisplay3dKinematics(bool display);

    bool displayHudDebug() const;
    void setDisplayHudDebug(bool display);

    int maxThrottleLimit() const;
    void setMaxThrottleLimit(int limit);

    float steeringSensitivity() const;
    void setSteeringSensitivity(float sensitivity);

    float lowBatteryWarningVolts() const;
    void setLowBatteryWarningVolts(float volts);

    float voltageScaleMultiplier() const;
    void setVoltageScaleMultiplier(float multiplier);

    int motorRpm() const { return m_motorRpm; }
    void setMotorRpm(int rpm);

    int wheelSizeMm() const { return m_wheelSizeMm; }
    void setWheelSizeMm(int size);

    float hudOpacity() const { return m_hudOpacity; }
    void setHudOpacity(float opacity);

    QString hudColor() const { return m_hudColor; }
    void setHudColor(const QString& color);

    float pitchOffset() const { return m_pitchOffset; }
    void setPitchOffset(float offset);

    float rollOffset() const { return m_rollOffset; }
    void setRollOffset(float offset);

    Q_INVOKABLE void calibrateLevel(float currentPitch, float currentRoll);
    Q_INVOKABLE void resetLevelCalibration();

signals:
    void radarPointLifetimeMsChanged();
    void sensorBaseAngleDegChanged();
    void invertTofChanged();
    void radarSweepSpeedChanged();
    void display3dKinematicsChanged();
    void displayHudDebugChanged();
    void maxThrottleLimitChanged();
    void steeringSensitivityChanged();
    void lowBatteryWarningVoltsChanged();
    void voltageScaleMultiplierChanged();

    void motorRpmChanged();
    void wheelSizeMmChanged();
    void hudOpacityChanged();
    void hudColorChanged();

    void pitchOffsetChanged();
    void rollOffsetChanged();

private:
    QSettings m_settings;
    int m_radarPointLifetimeMs;
    int m_sensorBaseAngleDeg;
    bool m_invertTof;
    int m_radarSweepSpeed;
    bool m_display3dKinematics;
    bool m_displayHudDebug;
    int m_maxThrottleLimit;
    float m_steeringSensitivity;
    float m_lowBatteryWarningVolts;
    float m_voltageScaleMultiplier;
    int m_motorRpm;
    int m_wheelSizeMm;
    float m_hudOpacity;
    QString m_hudColor;
    float m_pitchOffset;
    float m_rollOffset;
};
