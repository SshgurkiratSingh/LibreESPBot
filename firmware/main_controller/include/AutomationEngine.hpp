#pragma once
#include <cstdint>

class TB6612_Driver;
class DualVL53L0X;

class AutomationEngine {
public:
    AutomationEngine(TB6612_Driver* driver, DualVL53L0X* radar);
    void setAEB(bool enable);
    void setAPF(bool enable);
    void setAutoTurn(bool enable, int16_t heading);
    void update(int16_t &leftPwm, int16_t &rightPwm, float currentHeading = 0.0f);

private:
    TB6612_Driver* motorDriver;
    DualVL53L0X* tofRadar;
    bool aebEnabled;
    bool apfEnabled;
    bool autoTurnEnabled = false;
    int16_t targetHeading = 0;
    int stuckTicks = 0;
    float lastHeading = 0.0f;
    int turnSpeed = 350;
};
