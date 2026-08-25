#include "MpuDriver.hpp"
#include <math.h>

MpuDriver::MpuDriver() : _mpuAddr(0), _isInitialized(false) {}

bool MpuDriver::writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

bool MpuDriver::readBytes(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t n) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, n) != n) return false;
    for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
    return true;
}

bool MpuDriver::readByte(uint8_t addr, uint8_t reg, uint8_t& val) {
    return readBytes(addr, reg, &val, 1);
}

bool MpuDriver::devicePing(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

bool MpuDriver::findMPU() {
    // 0x68 (AD0 low) or 0x69 (AD0 high)
    const uint8_t addrs[] = { 0x68, 0x69 };
    for (uint8_t a : addrs) {
        if (!devicePing(a)) continue;
        uint8_t who = 0;
        if (!readByte(a, MPU_WHO_AM_I, who)) continue;
        if (who == a) {
            _mpuAddr = a;
            return true;
        }
    }
    return false;
}

bool MpuDriver::initMPU() {
    // Wake up: choose gyro X as the clock source
    if (!writeReg(_mpuAddr, REG_PWR_MGMT_1, 0x01)) return false;
    delay(10);
    // Sample rate = 8 kHz / (1 + SMPLRT_DIV) = 1 kHz.
    writeReg(_mpuAddr, REG_SMPLRT_DIV, 0x07);
    // DLPF = 0x04 -> ~21 Hz bandwidth (more stable, less noise).
    writeReg(_mpuAddr, REG_CONFIG, 0x04);
    // Gyro full scale ±250 °/s
    writeReg(_mpuAddr, REG_GYRO_CONFIG, 0x00);
    // Accel full scale ±2 g
    writeReg(_mpuAddr, REG_ACCEL_CONFIG, 0x00);
    // Data-ready interrupt
    writeReg(_mpuAddr, REG_INT_ENABLE, 0x01);
    return true;
}

bool MpuDriver::begin() {
    if (!findMPU()) return false;
    _isInitialized = initMPU();
    return _isInitialized;
}

bool MpuDriver::readRaw(int16_t& ax, int16_t& ay, int16_t& az, 
                        int16_t& gx, int16_t& gy, int16_t& gz, 
                        int16_t& temp) {
    if (!_isInitialized) return false;

    uint8_t d[14];
    if (!readBytes(_mpuAddr, REG_DATA_START, d, 14)) return false;

    auto i16 = [](uint8_t* p) -> int16_t {
        return (int16_t)((p[0] << 8) | p[1]);
    };

    ax   = i16(d + 0);
    ay   = i16(d + 2);
    az   = i16(d + 4);
    temp = i16(d + 6);
    gx   = i16(d + 8);
    gy   = i16(d + 10);
    gz   = i16(d + 12);
    
    return true;
}

bool MpuDriver::readScaled(float& ax_g, float& ay_g, float& az_g, 
                           float& gx_s, float& gy_s, float& gz_s, 
                           float& temp_c) {
    int16_t ax, ay, az, gx, gy, gz, temp;
    if (!readRaw(ax, ay, az, gx, gy, gz, temp)) return false;

    //  accel: ±2 g range  -> 16384 LSB/g
    //  gyro : ±250 °/s    -> 131  LSB/(°/s)
    //  temp : raw / 340 + 36.53 degrees C
    ax_g = ax / 16384.0f;
    ay_g = ay / 16384.0f;
    az_g = az / 16384.0f;
    gx_s = gx / 131.0f;
    gy_s = gy / 131.0f;
    gz_s = gz / 131.0f;
    temp_c = temp / 340.0f + 36.53f;

    return true;
}

void MpuDriver::getTilt(float& roll, float& pitch) {
    float ax, ay, az, gx, gy, gz, t;
    if (readScaled(ax, ay, az, gx, gy, gz, t)) {
        roll  = atan2f(ay, az) * 180.0f / PI;
        pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / PI;
    } else {
        roll = 0.0f;
        pitch = 0.0f;
    }
}
