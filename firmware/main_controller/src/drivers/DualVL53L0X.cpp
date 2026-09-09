#include "DualVL53L0X.hpp"
#include <Wire.h>

DualVL53L0X::DualVL53L0X() : tof1OK(false), tof2OK(false) {}

// ---------------------------------------------------------------------------
// Helper: safely begin() one sensor with an I2C read-back sanity check
// ---------------------------------------------------------------------------
bool DualVL53L0X::beginSensor(Adafruit_VL53L0X& lox, uint8_t addr, const char* label) {
    Wire.beginTransmission(addr);
    Wire.write(0xC0); // Model ID register
    if (Wire.endTransmission() != 0 || Wire.requestFrom((uint8_t)addr, (uint8_t)1) != 1) {
        Serial.printf("     [%s] I2C ping failed at 0x%02X\n", label, addr);
        Wire.read(); // drain
        return false;
    }
    Wire.read();

    bool ok = lox.begin(addr);
    if (!ok) {
        Serial.printf("     [%s] begin() FAILED at 0x%02X\n", label, addr);
        return false;
    }

    // --- KEY FIX: switch to CONTINUOUS ranging mode ---
    // The sensor now fires measurements autonomously at ~10 Hz (100ms cycle).
    // We just poll isRangeComplete() — zero blocking time in loop().
    ok = lox.startRangeContinuous(100); // 100ms period
    if (!ok) {
        Serial.printf("     [%s] startRangeContinuous() FAILED\n", label);
        return false;
    }
    Serial.printf("     [%s] online at 0x%02X — continuous mode active\n", label, addr);
    return true;
}

// ---------------------------------------------------------------------------
// init()
// ---------------------------------------------------------------------------
bool DualVL53L0X::init() {
    pinMode(XSHUT_1, OUTPUT);
    pinMode(XSHUT_2, OUTPUT);

    // Put both sensors in hard reset
    digitalWrite(XSHUT_1, LOW);
    digitalWrite(XSHUT_2, LOW);
    delay(100);

    // Find any sensor that is ignoring XSHUT (stuck always-on)
    uint8_t stuckAddr = 0;
    uint8_t possibleAddrs[] = {0x29, 0x30, 0x31};
    for (int i = 0; i < 3; ++i) {
        Wire.beginTransmission(possibleAddrs[i]);
        if (Wire.endTransmission() == 0) {
            stuckAddr = possibleAddrs[i];
            break;
        }
    }

    if (stuckAddr != 0) {
        Serial.printf("  -> Stuck sensor at 0x%02X. Renaming...\n", stuckAddr);

        // Move stuck sensor back to 0x29 if it wandered to another address
        if (stuckAddr != 0x29) {
            Wire.beginTransmission(stuckAddr);
            Wire.write(0x8A); // VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS
            Wire.write((0x29 & 0x7F) * 2);
            Wire.endTransmission();
            delay(10);
        }

        // Begin stuck sensor (always-on, at 0x29), then move it to 0x30
        if (beginSensor(lox2, 0x29, "Stuck")) {
            tof2OK = lox2.setAddress(VL53_ADDR_1); // -> 0x30
            Serial.printf(tof2OK ? "     Stuck sensor at 0x%02X\n" : "     setAddress FAILED\n", VL53_ADDR_1);
            if (!tof2OK) lox2.stopRangeContinuous();
        }

        // Release the XSHUT-controlled sensor — it boots at 0x29
        digitalWrite(XSHUT_1, HIGH);
        digitalWrite(XSHUT_2, HIGH);
        delay(150);

        tof1OK = beginSensor(lox1, VL53_ADDR_2 /*0x29*/, "XSHUT");

    } else {
        Serial.println("  -> No stuck sensor. Normal XSHUT sequencing.");

        // Sensor 1: release, begin at 0x29, move to 0x30
        digitalWrite(XSHUT_1, HIGH);
        delay(150);
        if (beginSensor(lox1, 0x29, "Sensor1")) {
            tof1OK = lox1.setAddress(VL53_ADDR_1); // -> 0x30
            if (!tof1OK) lox1.stopRangeContinuous();
        }

        // Sensor 2: release, begin at 0x29 (stays there)
        digitalWrite(XSHUT_2, HIGH);
        delay(150);
        tof2OK = beginSensor(lox2, VL53_ADDR_2 /*0x29*/, "Sensor2");
    }

    return (tof1OK || tof2OK);
}

// ---------------------------------------------------------------------------
// update() — called every loop(), NEVER blocks
// ---------------------------------------------------------------------------
void DualVL53L0X::update() {
    // Only poll the sensor over I2C every 20ms to prevent bus starvation
    static unsigned long lastPollTime = 0;
    if (millis() - lastPollTime < 20) return;
    lastPollTime = millis();

    // Sensor 1 (Left)
    if (tof1OK) {
        if (lox1.isRangeComplete()) {
            uint16_t r = lox1.readRangeResult();
            // readRangeResult returns 0xffff on phase fail / no target
            m_dist1 = (r < 8000) ? r : 0;
        }
    }

    // Sensor 2 (Right)
    if (tof2OK) {
        if (lox2.isRangeComplete()) {
            uint16_t r = lox2.readRangeResult();
            m_dist2 = (r < 8000) ? r : 0;
        }
    }
}


// ---------------------------------------------------------------------------
// Getters — return cached values, updated by update()
// ---------------------------------------------------------------------------
uint16_t DualVL53L0X::getLeftDistanceMm() {
    return tof1OK ? m_dist1 : 0;
}

uint16_t DualVL53L0X::getRightDistanceMm() {
    return tof2OK ? m_dist2 : 0;
}
