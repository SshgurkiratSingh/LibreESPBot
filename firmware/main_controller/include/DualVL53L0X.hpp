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

    // Per-sensor availability (so the caller can zero/report failed sensors)
    bool leftAvailable()  const { return tof1OK; }
    bool rightAvailable() const { return tof2OK; }

private:
    Adafruit_VL53L0X lox1;
    Adafruit_VL53L0X lox2;
    
    bool tof1OK;
    bool tof2OK;
    
    // GPIO 25/26 conflict with the TB6612 motor pins (PWMA/AIN1), so use
    // free GPIOs 16/17 for the ToF reset lines instead.
    static const uint8_t XSHUT_1 = 23;
    static const uint8_t XSHUT_2 = 19;
    
    static const uint8_t VL53_ADDR_1 = 0x30;
    static const uint8_t VL53_ADDR_2 = 0x29;

    uint16_t readVL53(Adafruit_VL53L0X& lox);
};
