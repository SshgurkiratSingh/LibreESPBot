#include "DualVL53L0X.hpp"
#include <Wire.h>

DualVL53L0X::DualVL53L0X() : tof1OK(false), tof2OK(false) {}

bool DualVL53L0X::init() {
    pinMode(XSHUT_1, OUTPUT);
    pinMode(XSHUT_2, OUTPUT);

    // --- Step 0: put BOTH modules into reset (XSHUT LOW) ----------------
    digitalWrite(XSHUT_1, LOW);
    digitalWrite(XSHUT_2, LOW);
    delay(300);

    // Find the stuck sensor's current address.
    // DO NOT scan the whole bus (1-127), because the Compass (0x0D, 0x1E, 0x2C)
    // and MPU6050 (0x68) are also on the bus and will respond!
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
        // --- A sensor ignores XSHUT -> it is always on at stuckAddr ----------
        Serial.printf("  -> Stuck sensor found at 0x%02X. Renaming it...\n", stuckAddr);
        
        // If the stuck sensor is not at 0x29 (e.g. from a previous boot), we MUST move it 
        // back to 0x29 before calling begin(). Adafruit's begin() sends a soft reset which 
        // instantly forces the sensor back to 0x29. If we called begin(0x31), all subsequent 
        // config commands would be sent to 0x31 (which is now dead), resulting in 26 seconds of timeouts!
        if (stuckAddr != 0x29) {
            Serial.println("     Temporarily returning stuck sensor to 0x29 to prevent begin() timeouts...");
            Wire.beginTransmission(stuckAddr);
            Wire.write(0x8A); // VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS
            Wire.write(0x29);
            Wire.endTransmission();
            delay(10);
            stuckAddr = 0x29;
        }
        
        tof2OK = lox2.begin(0x29);
        if (tof2OK) {
            tof2OK = lox2.setAddress(VL53_ADDR_1); // Move to 0x30
            Serial.printf(tof2OK ? "     Stuck sensor safely moved to 0x%02X.\n" : "     setAddress() FAILED.\n", VL53_ADDR_1);
        } else {
            Serial.println("     Stuck sensor begin() FAILED.");
        }

        // Now release the working-XSHUT sensor - it boots back at 0x29.
        digitalWrite(XSHUT_1, HIGH);
        digitalWrite(XSHUT_2, HIGH);
        delay(300);
        
        tof1OK = lox1.begin(VL53_ADDR_2); // 0x29
        Serial.println(tof1OK ? "     Working-XSHUT sensor online at 0x29." : "     Working sensor begin() FAILED.");
    } else {
        // --- No stuck sensor: BOTH XSHUTs work. ---------------------------
        Serial.println("  -> No stuck sensor. Both XSHUTs work; normal sequencing.");
        
        digitalWrite(XSHUT_1, HIGH);
        delay(300);
        tof1OK = lox1.begin(VL53_ADDR_2); // 0x29
        if (tof1OK) {
            tof1OK = lox1.setAddress(VL53_ADDR_1); // move to 0x30
            Serial.println(tof1OK ? "     Sensor 1 moved to 0x30." : "     Sensor 1 setAddress() FAILED.");
        } else {
            Serial.println("     Sensor 1 begin() FAILED.");
        }

        digitalWrite(XSHUT_2, HIGH);
        delay(300);
        tof2OK = lox2.begin(VL53_ADDR_2); // 0x29
        Serial.println(tof2OK ? "     Sensor 2 online at 0x29." : "     Sensor 2 begin() FAILED.");
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
