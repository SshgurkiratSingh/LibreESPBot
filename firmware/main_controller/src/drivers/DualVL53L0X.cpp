#include "DualVL53L0X.hpp"
#include <Wire.h>

DualVL53L0X::DualVL53L0X() : tof1OK(false), tof2OK(false) {}

bool DualVL53L0X::init() {
    pinMode(XSHUT_1, OUTPUT);
    pinMode(XSHUT_2, OUTPUT);

    // Hold both in reset (XSHUT low) so neither is on the I2C bus yet.
    digitalWrite(XSHUT_1, LOW);
    digitalWrite(XSHUT_2, LOW);
    delay(20);

    // Sensor #1: wake it, init at default 0x29, then move it to 0x30.
    digitalWrite(XSHUT_1, HIGH);
    delay(20);
    tof1OK = lox1.begin();
    if (tof1OK) {
        lox1.setAddress(VL53_ADDR_1);
        Serial.printf("  VL53L0X #1 ready at 0x%02X\n", VL53_ADDR_1);
    } else {
        Serial.println("  VL53L0X #1 init FAILED");
    }

    // Sensor #2: wake up and init at the (now free) default 0x29.
    digitalWrite(XSHUT_2, HIGH);
    delay(20);
    tof2OK = lox2.begin();
    if (tof2OK) {
        Serial.printf("  VL53L0X #2 ready at 0x%02X\n", VL53_ADDR_2);
    } else {
        Serial.println("  VL53L0X #2 init FAILED");
    }

    return (tof1OK || tof2OK);
}

uint16_t DualVL53L0X::readVL53(Adafruit_VL53L0X& lox) {
    VL53L0X_RangingMeasurementData_t m;
    lox.rangingTest(&m, false); // false = no debug output
    
    // 4 = phase fail / out of range
    if (m.RangeStatus != 4) {
        return m.RangeMilliMeter;
    }
    return 0;
}

uint16_t DualVL53L0X::getLeftDistanceMm() {
    if (tof1OK) {
        return readVL53(lox1);
    }
    return 0;
}

uint16_t DualVL53L0X::getRightDistanceMm() {
    if (tof2OK) {
        return readVL53(lox2);
    }
    return 0;
}
