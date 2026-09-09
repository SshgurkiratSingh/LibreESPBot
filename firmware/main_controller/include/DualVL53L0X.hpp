#pragma once
#include <Arduino.h>
#include <cstdint>
#include <Adafruit_VL53L0X.h>

class DualVL53L0X {
public:
    DualVL53L0X();
    bool init();

    // Call this every loop() — reads new measurements non-blocking if ready
    void update();

    // Returns the last cached distance in mm, or 0 if out of range / failed
    uint16_t getLeftDistanceMm();
    uint16_t getRightDistanceMm();

    // Per-sensor availability
    bool leftAvailable()  const { return tof1OK; }
    bool rightAvailable() const { return tof2OK; }

private:
    bool beginSensor(Adafruit_VL53L0X& lox, uint8_t addr, const char* label);

    Adafruit_VL53L0X lox1;
    Adafruit_VL53L0X lox2;

    bool tof1OK;
    bool tof2OK;

    // Last valid cached readings (updated by update())
    uint16_t m_dist1 = 0;
    uint16_t m_dist2 = 0;

    // GPIO for XSHUT reset lines
    static const uint8_t XSHUT_1 = 16;
    static const uint8_t XSHUT_2 = 4;

    static const uint8_t VL53_ADDR_1 = 0x30;
    static const uint8_t VL53_ADDR_2 = 0x29;
};
