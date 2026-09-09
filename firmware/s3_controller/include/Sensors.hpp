#pragma once

#include <Arduino.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_VL53L0X.h>

// ==========================================
// IR Array Pins
// ==========================================
#define IR_PIN_1 2
#define IR_PIN_2 3
#define IR_PIN_3 4
#define IR_PIN_4 5
#define IR_PIN_5 6
#define IR_PIN_6 7
#define IR_PIN_7 8

// ==========================================
// Hardware I2C (for BMP280)
// ==========================================
#define I2C_SDA 47
#define I2C_SCL 48

// ==========================================
// Software I2C (for VL53L0X)
// ==========================================
#define TOF1_SDA 15
#define TOF1_SCL 16

#define TOF2_SDA 17
#define TOF2_SCL 18

#include "MpuDriver.hpp"
#include "CompassDriver.hpp"

class S3Sensors {
public:
    S3Sensors();
    void init();
    
    // IR Array
    uint8_t readIRArray();
    
    // Barometer
    float getTemperature();
    float getPressure();
    
    // ToF Sensors
    uint16_t getLeftDistanceMm();
    uint16_t getRightDistanceMm();

    // Orientation
    MpuDriver imu;
    CompassDriver compass;
    bool imuOk;
    bool compassOk;

private:
    Adafruit_BMP280 bmp;
    bool bmpOk;

    Adafruit_VL53L0X lox1;
    Adafruit_VL53L0X lox2;
    
    bool tof1Ok;
    bool tof2Ok;

    uint8_t irPins[7] = {IR_PIN_1, IR_PIN_2, IR_PIN_3, IR_PIN_4, IR_PIN_5, IR_PIN_6, IR_PIN_7};
};
