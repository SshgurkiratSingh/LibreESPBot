#pragma once
#include <Arduino.h>
#include <cstdint>
#include <Adafruit_VL53L0X.h>

class DualVL53L0X {
public:
    DualVL53L0X();
    bool init();
    
    // Returns distance in mm, or 0 if out of range / failed
    uint16_t getLeftDistanceMm();
    uint16_t getRightDistanceMm();

private:
    Adafruit_VL53L0X lox1;
    Adafruit_VL53L0X lox2;
    
    bool tof1OK;
    bool tof2OK;
    
    static const uint8_t XSHUT_1 = 25;
    static const uint8_t XSHUT_2 = 26;
    
    static const uint8_t VL53_ADDR_1 = 0x30;
    static const uint8_t VL53_ADDR_2 = 0x29;

    uint16_t readVL53(Adafruit_VL53L0X& lox);
};
