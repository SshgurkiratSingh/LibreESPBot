#pragma once
#include <cstdint>

class TB6612_Driver;
class DualVL53L0X;

class AutomationEngine {
public:
    AutomationEngine(TB6612_Driver* driver, DualVL53L0X* radar);
    void setAEB(bool enable);
    void setAPF(bool enable);
    void update();

private:
    TB6612_Driver* motorDriver;
    DualVL53L0X* tofRadar;
    bool aebEnabled;
    bool apfEnabled;
};
