#pragma once
#include <Arduino.h>
#include <Wire.h>

class MpuDriver {
public:
    MpuDriver();
    bool begin();
    
    // Read raw 16-bit values from the MPU
    bool readRaw(int16_t& ax, int16_t& ay, int16_t& az, 
                 int16_t& gx, int16_t& gy, int16_t& gz, 
                 int16_t& temp);

    // Read scaled values (g for accel, deg/s for gyro, C for temp)
    bool readScaled(float& ax_g, float& ay_g, float& az_g, 
                    float& gx_s, float& gy_s, float& gz_s, 
                    float& temp_c);

    // Basic tilt estimation directly from accelerometer
    void getTilt(float& roll, float& pitch);

    uint8_t getAddress() const { return _mpuAddr; }

private:
    uint8_t _mpuAddr;
    bool _isInitialized;

    // Registers
    static const uint8_t REG_PWR_MGMT_1   = 0x6B;
    static const uint8_t REG_SMPLRT_DIV   = 0x19;
    static const uint8_t REG_CONFIG       = 0x1A;
    static const uint8_t REG_GYRO_CONFIG  = 0x1B;
    static const uint8_t REG_ACCEL_CONFIG = 0x1C;
    static const uint8_t REG_INT_ENABLE   = 0x38;
    static const uint8_t REG_DATA_START   = 0x3B;
    static const uint8_t MPU_WHO_AM_I     = 0x75;

    bool writeReg(uint8_t addr, uint8_t reg, uint8_t val);
    bool readBytes(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t n);
    bool readByte(uint8_t addr, uint8_t reg, uint8_t& val);
    bool devicePing(uint8_t addr);
    
    bool findMPU();
    bool initMPU();
};
