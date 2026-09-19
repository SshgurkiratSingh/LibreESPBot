#pragma once
#include <cstdint>

class TB6612_Driver {
public:
    TB6612_Driver();
    void init();
    void setMotorLeft(int16_t speed);
    void setMotorRight(int16_t speed);
    
    int16_t getCurrentLeftPwm() const { return currentLeft; }
    int16_t getCurrentRightPwm() const { return currentRight; }

private:
    int16_t applyDeadband(int16_t u);
    
    int16_t currentLeft = 0;
    int16_t currentRight = 0;
};
