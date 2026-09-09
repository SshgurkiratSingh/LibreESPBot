#include "Sensors.hpp"
#include <Wire.h>

S3Sensors::S3Sensors() : bmpOk(false), tof1Ok(false), tof2Ok(false)
{}

void S3Sensors::init() {
    // 1. Initialize IR Array
    for (int i = 0; i < 7; i++) {
        pinMode(irPins[i], INPUT);
    }
    Serial.println("IR Array Initialized on pins 2-8.");

    // 2. Initialize Hardware I2C (BMP280)
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Default address for BMP280 is usually 0x76 or 0x77
    if (!bmp.begin(0x76) && !bmp.begin(0x77)) {
        Serial.println("Could not find a valid BMP280 sensor, check wiring!");
        bmpOk = false;
    } else {
        bmpOk = true;
        /* Default settings from datasheet. */
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                        Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                        Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                        Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                        Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
        Serial.println("BMP280 Initialized.");
    }

    // 3. Initialize I2C (VL53L0X)
    Wire1.begin(TOF2_SDA, TOF2_SCL);

    // TOF 1 (Uses `Wire`, sharing bus with BMP280, MPU6050, Compass)
    if (!lox1.begin(VL53L0X_I2C_ADDR, false, &Wire)) {
        Serial.println("Failed to boot VL53L0X on I2C Bus 0");
        tof1Ok = false;
    } else {
        tof1Ok = true;
        Serial.println("VL53L0X 1 Initialized.");
    }

    // TOF 2 (Uses `Wire1`, independent bus)
    if (!lox2.begin(VL53L0X_I2C_ADDR, false, &Wire1)) {
        Serial.println("Failed to boot VL53L0X on I2C Bus 1");
        tof2Ok = false;
    } else {
        tof2Ok = true;
        Serial.println("VL53L0X 2 Initialized.");
    }

    // 4. Initialize MPU6050 and Compass (using Hardware I2C `Wire` which was begun with I2C_SDA/I2C_SCL above)
    imuOk = imu.begin();
    if (imuOk) {
        Serial.println("MPU6050 Initialized.");
    } else {
        Serial.println("Failed to boot MPU6050.");
    }

    compassOk = compass.begin();
    if (compassOk) {
        Serial.println("Compass Initialized.");
    } else {
        Serial.println("Failed to boot Compass.");
    }
}

uint8_t S3Sensors::readIRArray() {
    uint8_t state = 0;
    for (int i = 0; i < 7; i++) {
        // Read each pin. Assuming active HIGH for line detection (white/black depending on sensor)
        if (digitalRead(irPins[i]) == HIGH) {
            state |= (1 << i);
        }
    }
    return state;
}

float S3Sensors::getTemperature() {
    if (bmpOk) return bmp.readTemperature();
    return 0.0f;
}

float S3Sensors::getPressure() {
    if (bmpOk) return bmp.readPressure();
    return 0.0f;
}

uint16_t S3Sensors::getLeftDistanceMm() {
    if (!tof1Ok) return 0;
    VL53L0X_RangingMeasurementData_t m;
    lox1.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
        return m.RangeMilliMeter;
    }
    return 0;
}

uint16_t S3Sensors::getRightDistanceMm() {
    if (!tof2Ok) return 0;
    VL53L0X_RangingMeasurementData_t m;
    lox2.rangingTest(&m, false);
    if (m.RangeStatus != 4) {
        return m.RangeMilliMeter;
    }
    return 0;
}
