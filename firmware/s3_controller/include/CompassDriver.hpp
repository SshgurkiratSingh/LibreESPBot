#pragma once
#include <Arduino.h>
#include <Wire.h>

class CompassDriver {
public:
    enum MagType { MAG_NONE, MAG_HMC, MAG_QMC };

    CompassDriver();
    bool begin();
    bool readMag(int16_t& x, int16_t& y, int16_t& z);
    float getHeading();

    MagType getType() const { return _chipType; }

private:
    MagType _chipType;
    uint8_t _chipAddr;

    int16_t _minX = 32767;
    int16_t _maxX = -32768;
    int16_t _minY = 32767;
    int16_t _maxY = -32768;

    bool writeReg(uint8_t addr, uint8_t reg, uint8_t val);
    bool readBytes(uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t n);
    bool readByte(uint8_t addr, uint8_t reg, uint8_t& val);
    bool devicePing(uint8_t addr);

    bool isHMC(uint8_t addr);
    bool isQMC(uint8_t addr);
    MagType findMag();
    bool initMag();
};
