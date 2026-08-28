#pragma once
#include <cstdint>

class TB6612_Driver {
public:
    TB6612_Driver();
    void init();
    void setMotorLeft(int16_t speed);
    void setMotorRight(int16_t speed);

private:
    int16_t applyDeadband(int16_t u);
    
    int16_t currentLeft = 0;
    int16_t currentRight = 0;
    static const int16_t MAX_SLEW_STEP = 50; // Maximum PWM change per 50Hz update (1000ms from 0 to 1000 = 50 per 20ms)
};
