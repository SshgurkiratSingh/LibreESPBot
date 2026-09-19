#pragma once

#include <Arduino.h>
#include <Wire.h>

class MPU6050 {
public:
    enum AccelRange { AFS_2G = 0, AFS_4G, AFS_8G, AFS_16G };
    enum GyroRange { GFS_250 = 0, GFS_500, GFS_1000, GFS_2000 };

    struct Data {
        int16_t ax, ay, az;
        int16_t temp;
        int16_t gx, gy, gz;
    };

    explicit MPU6050(uint8_t address = 0x68);

    void setAccelRange(AccelRange r);
    void setGyroRange(GyroRange r);
    void setClockSource(uint8_t s);

    float accelScale() const;
    float gyroScale() const;

    bool begin();
    void wake();

    bool read(Data& d);

    // Helpers for ESP266-Car telemetry
    bool readScaled(float& ax_g, float& ay_g, float& az_g, 
                    float& gx_s, float& gy_s, float& gz_s, 
                    float& temp_c);
    void getTilt(float& roll, float& pitch);

private:
    uint8_t readReg(uint8_t reg) const;
    bool readRegs(uint8_t reg, uint8_t* buf, uint8_t len) const;

    uint8_t addr_;
    AccelRange accelRange_;
    GyroRange gyroRange_;
    uint8_t clockSource_;
};
