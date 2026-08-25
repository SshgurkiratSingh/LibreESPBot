#include "CompassDriver.hpp"

CompassDriver::CompassDriver() : _chipType(MAG_NONE), _chipAddr(0) {}

bool CompassDriver::writeReg(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

bool CompassDriver::readBytes(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t n) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, n) != n) return false;
    for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
    return true;
}

bool CompassDriver::readByte(uint8_t addr, uint8_t reg, uint8_t& val) {
    return readBytes(addr, reg, &val, 1);
}

bool CompassDriver::devicePing(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

bool CompassDriver::isHMC(uint8_t addr) {
    uint8_t id[3];
    if (!readBytes(addr, 0x0A, id, 3)) return false;
    return (id[0] == 'H' && id[1] == '4' && id[2] == '3');
}

bool CompassDriver::isQMC(uint8_t addr) {
    uint8_t id = 0;
    if (!readByte(addr, 0x00, id)) return false;
    return (id == 0x80);
}

CompassDriver::MagType CompassDriver::findMag() {
    const uint8_t addrs[] = {0x2C, 0x1E, 0x0D, 0x1A, 0x1C};
    for (uint8_t a : addrs) {
        if (!devicePing(a)) continue;
        Serial.printf("  Probing 0x%02X ... ", a);

        if (isQMC(a)) {
            Serial.println("QMC5883L/P-compatible chip found");
            _chipAddr = a; _chipType = MAG_QMC; return _chipType;
        }
        if (isHMC(a)) {
            Serial.println("HMC5883L chip found");
            _chipAddr = a; _chipType = MAG_HMC; return _chipType;
        }
        if (a == 0x2C) {
            Serial.println("no id match, but 0x2C is the QMC5883P address -> using it");
            _chipAddr = a; _chipType = MAG_QMC; return _chipType;
        }
        Serial.println("unknown device (not a magnetometer we know)");
    }
    _chipType = MAG_NONE;
    return _chipType;
}

bool CompassDriver::initMag() {
    if (_chipType == MAG_QMC) {
        writeReg(_chipAddr, 0x0A, (0x03) | (0x02 << 2) | (0x00 << 4));
        return writeReg(_chipAddr, 0x0B, (0x02 << 2) | 0x00);
    }
    if (_chipType == MAG_HMC) {
        writeReg(_chipAddr, 0x00, 0x78);
        writeReg(_chipAddr, 0x01, 0x20);
        return writeReg(_chipAddr, 0x02, 0x00);
    }
    return false;
}

bool CompassDriver::begin() {
    if (findMag() == MAG_NONE) {
        return false;
    }
    return initMag();
}

bool CompassDriver::readMag(int16_t& x, int16_t& y, int16_t& z) {
    uint8_t d[6];
    if (_chipType == MAG_QMC) {
        if (!readBytes(_chipAddr, 0x01, d, 6)) return false;
        x = (int16_t)((d[1] << 8) | d[0]);
        y = (int16_t)((d[3] << 8) | d[2]);
        z = (int16_t)((d[5] << 8) | d[4]);
        return true;
    }
    if (_chipType == MAG_HMC) {
        if (!readBytes(_chipAddr, 0x03, d, 6)) return false;
        x = (int16_t)((d[0] << 8) | d[1]);
        y = (int16_t)((d[4] << 8) | d[5]);
        z = (int16_t)((d[2] << 8) | d[3]);
        return true;
    }
    return false;
}

float CompassDriver::getHeading() {
    int16_t x, y, z;
    if (!readMag(x, y, z)) return 0.0f;
    float h = atan2f(y, x) * 180.0f / PI;
    if (h < 0.0f) h += 360.0f;
    return h;
}
