#include "MPU6050.hpp"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

// ---------------------------------------------------------------------------
// MPU-6050 register map (subset used by this driver).
// ---------------------------------------------------------------------------
#define MPU_REG_SMPLRT_DIV    0x19
#define MPU_REG_CONFIG         0x1A
#define MPU_REG_GYRO_CONFIG    0x1B
#define MPU_REG_ACCEL_CONFIG   0x1C
#define MPU_REG_PWR_MGMT_1     0x6B
#define MPU_REG_WHO_AM_I       0x75
#define MPU_REG_ACCEL_XOUT_H   0x3B   // burst start; 14 bytes => TEMP/GYRO

MPU6050::MPU6050(uint8_t address)
    : addr_(address),
      accelRange_(AFS_2G),
      gyroRange_(GFS_250),
      clockSource_(1) {}

void MPU6050::setAccelRange(AccelRange r) { accelRange_ = r; }
void MPU6050::setGyroRange(GyroRange r)  { gyroRange_ = r; }
void MPU6050::setClockSource(uint8_t s)  { clockSource_ = s; }

float MPU6050::accelScale() const {
  // LSB resolution: 16384 / 8192 / 4096 / 2048 for 2/4/8/16 g.
  static const float scales[4] = {16384.0f, 8192.0f, 4096.0f, 2048.0f};
  return scales[accelRange_];
}

float MPU6050::gyroScale() const {
  // LSB resolution: 131 / 65.5 / 32.8 / 16.4 for 250/500/1000/2000 dps.
  static const float scales[4] = {131.0f, 65.5f, 32.8f, 16.4f};
  return scales[gyroRange_];
}

uint8_t MPU6050::readReg(uint8_t reg) const {
  uint8_t val = 0;
  readRegs(reg, &val, 1);
  return val;
}

bool MPU6050::readRegs(uint8_t reg, uint8_t* buf, uint8_t len) const {
  Wire.beginTransmission(addr_);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom((int)addr_, (int)len) != len) return false;
  for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

bool MPU6050::begin() {
  // Write a configuration burst to the power-management register:
  // clear SLEEP bit and select the clock source.
  Wire.beginTransmission(addr_);
  Wire.write(MPU_REG_PWR_MGMT_1);
  Wire.write(clockSource_);
  Wire.endTransmission();

  Wire.beginTransmission(addr_);
  Wire.write(MPU_REG_ACCEL_CONFIG);
  Wire.write((uint8_t)accelRange_ << 3);
  Wire.endTransmission();

  Wire.beginTransmission(addr_);
  Wire.write(MPU_REG_GYRO_CONFIG);
  Wire.write((uint8_t)gyroRange_ << 3);
  Wire.endTransmission();

  return readReg(MPU_REG_WHO_AM_I) == 0x68;
}

void MPU6050::wake() {
  Wire.beginTransmission(addr_);
  Wire.write(MPU_REG_PWR_MGMT_1);
  Wire.write(clockSource_);
  Wire.endTransmission();
}

bool MPU6050::read(Data& d) {
  Wire.beginTransmission(addr_);
  Wire.write(MPU_REG_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom((int)addr_, (int)14) != 14) return false;

  int16_t* raw[7] = {&d.ax, &d.ay, &d.az, &d.temp, &d.gx, &d.gy, &d.gz};
  for (int i = 0; i < 7; i++) {
    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    *raw[i] = (int16_t)((hi << 8) | lo);
  }
  return true;
}

bool MPU6050::readScaled(float& ax_g, float& ay_g, float& az_g, 
                         float& gx_s, float& gy_s, float& gz_s, 
                         float& temp_c) {
    Data d;
    if (!read(d)) return false;

    ax_g = d.ax / accelScale();
    ay_g = d.ay / accelScale();
    az_g = d.az / accelScale();
    
    gx_s = d.gx / gyroScale();
    gy_s = d.gy / gyroScale();
    gz_s = d.gz / gyroScale();

#ifdef ESP32
    // Use internal temperature sensor as requested
    temp_c = temperatureRead();
#else
    temp_c = d.temp / 340.0f + 36.53f;
#endif

    return true;
}

void MPU6050::getTilt(float& roll, float& pitch) {
    float ax, ay, az, gx, gy, gz, t;
    if (readScaled(ax, ay, az, gx, gy, gz, t)) {
        roll  = atan2f(ay, az) * 180.0f / PI;
        pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / PI;
    } else {
        roll = 0.0f;
        pitch = 0.0f;
    }
}
