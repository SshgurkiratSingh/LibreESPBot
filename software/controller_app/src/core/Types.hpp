#pragma once
#include <cstdint>

#pragma pack(push, 1)

// Telemetry Frame sent by Rover at 50 Hz (MCU -> App)
struct VehicleTelemetryPacket {
    uint16_t preamble;        // 0xAA55
    uint8_t  hardwareRev;     // Hardware profile ID
    uint8_t  activeImuType;   // 0x01: MPU6050, 0x02: BMI160
    uint8_t  activeMagType;   // 0x01: QMC5883L, 0x02: HMC5883L, 0x03: LIS3MDL
    uint32_t timestampMs;     // System uptime in milliseconds

    // Kinematics & Orientation
    float pitchDeg;
    float rollDeg;
    float yawDeg;
    float headingCompassDeg;
    float linearAccX;
    float linearAccY;
    float linearAccZ;

    // Pan Scanner State
    int16_t  servoAngleDeg;   // -90 to +90 degrees
    uint16_t tof1DistMm;      // Distance from Left ToF (0x30)
    uint16_t tof2DistMm;      // Distance from Right ToF (0x31)

    // Actuation & Power
    int16_t  motorLeftPwm;    // -1023 to +1023
    int16_t  motorRightPwm;   // -1023 to +1023
    float    batteryVoltage;  // Scaled voltage (e.g. 11.1V - 12.6V)
    float    imuTempC;        // MPU-6050 Temperature in Celsius

    // Status Flags (Bit 0: Obstacle, Bit 1: Braking, Bit 2: Radar Active, Bit 3: FailSafe)
    uint16_t statusFlags;
    uint16_t crc16;           // CRC-16-CCITT across entire struct except crc16
};

// Actuation Frame sent by Controller at 50 Hz (App -> Rover)
struct VehicleCommandPacket {
    uint16_t preamble;        // 0x55AA
    uint16_t sequenceId;
    int16_t  throttleAxis;    // -1023 to +1023
    int16_t  steeringAxis;    // -1023 to +1023
    
    // Feature Configuration Flags
    uint8_t  enableAutoBrake;
    uint8_t  enableApfAvoidance;
    uint8_t  enableRadarSweep;
    uint8_t  radarSweepSpeed; // Servo sweeping speed control
    uint8_t  speedModeLimit;  // 0: Precision (30%), 1: Normal (70%), 2: Sport (100%)
    uint8_t  headlightMode;   // 0: Off, 1: On (White), 2: Police Strobe, 3: Custom
    uint8_t  customLedR;
    uint8_t  customLedG;
    uint8_t  customLedB;
    uint8_t  customLedPattern; // 8-bit blinking sequence
    uint8_t  enableNoLagMode; // Hard real-time mode (disables non-essential tasks)
    
    uint16_t crc16;
};

#pragma pack(pop)
