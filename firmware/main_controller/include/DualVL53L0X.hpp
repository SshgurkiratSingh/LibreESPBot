#pragma once
#include <cstdint>

class DualVL53L0X {
public:
    DualVL53L0X();
    void init();
    uint16_t getLeftDistanceMm();
    uint16_t getRightDistanceMm();
};
