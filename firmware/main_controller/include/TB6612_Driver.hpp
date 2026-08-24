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
};
