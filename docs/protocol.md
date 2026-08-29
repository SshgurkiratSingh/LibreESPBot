# Communication Protocol

## UDP Zero-Copy Binary Telemetry & Command Link

The ESP32 Rover Platform communicates with the LibreESPBot exclusively via UDP datagrams over a local Wi-Fi network. To minimize processing overhead and latency on both the ESP32 and the controller, data is serialized into packed C-structs. This avoids the heavy serialization/deserialization penalties associated with JSON or XML.

### Transmission Rates
- **Rover -> Controller (Telemetry):** 50 Hz (Port 8889)
- **Controller -> Rover (Actuation):** 50 Hz (Port 8888)

---

## 1. CRC-16-CCITT Validation

Both telemetry and command packets employ a CRC-16-CCITT checksum for data integrity validation over the unreliable UDP transport layer.

- **Polynomial**: `0x1021` ( $G(x) = x^{16} + x^{12} + x^5 + 1$ )
- **Initial Value**: `0xFFFF`
- **Scope**: Calculated across the entire packet structure, *excluding* the final `crc16` field itself.

Packets failing the CRC check are immediately discarded.

---

## 2. Vehicle Telemetry Packet (Rover -> Controller)

This packet contains the absolute state of the rover's kinematics, sensors, and power systems.

```cpp
#pragma pack(push, 1)
struct VehicleTelemetryPacket {
    uint16_t preamble;        // 0xAA55
    uint8_t  hardwareRev;     // Hardware profile ID
    uint8_t  activeImuType;   // 0x01: MPU6050, 0x02: BMI160
    uint8_t  activeMagType;   // 0x01: QMC5883L, 0x02: HMC5883L, 0x03: LIS3MDL
    uint32_t timestampMs;     // System uptime in milliseconds

    // Kinematics & Orientation
    float pitchDeg;           // -90.0 to +90.0 degrees
    float rollDeg;            // -180.0 to +180.0 degrees
    float yawDeg;             // -180.0 to +180.0 degrees
    float headingCompassDeg;  // 0.0 to 360.0 degrees
    float linearAccX;         // m/s^2
    float linearAccY;         // m/s^2
    float linearAccZ;         // m/s^2

    // Pan Scanner State
    int16_t  servoAngleDeg;   // -90 to +90 degrees relative to center
    uint16_t tof1DistMm;      // Left ToF (0x30) distance in mm
    uint16_t tof2DistMm;      // Right ToF (0x31) distance in mm

    // Actuation & Power
    int16_t  motorLeftPwm;    // -1023 to +1023 applied PWM
    int16_t  motorRightPwm;   // -1023 to +1023 applied PWM
    float    batteryVoltage;  // 11.1V - 12.6V scaled LiPo voltage

    // Status Flags (Bitwise)
    // Bit 0: Obstacle Detected (AEB triggerable)
    // Bit 1: Actively Braking
    // Bit 2: Radar Sweep Active
    // Bit 3: FailSafe Mode Active (Connection Lost)
    uint16_t statusFlags;
    
    uint16_t crc16;           // Validation Checksum
};
#pragma pack(pop)
```

---

## 3. Vehicle Command Packet (Controller -> Rover)

This packet transmits continuous control axis values and feature toggle states to the rover.

```cpp
#pragma pack(push, 1)
struct VehicleCommandPacket {
    uint16_t preamble;        // 0x55AA
    uint16_t sequenceId;      // Increments every packet to detect drops
    
    int16_t  throttleAxis;    // -1023 (Full Reverse) to +1023 (Full Forward)
    int16_t  steeringAxis;    // -1023 (Hard Left) to +1023 (Hard Right)
    
    // Feature Configuration Flags
    uint8_t  enableAutoBrake;    // 1: Enable AEB, 0: Disable
    uint8_t  enableApfAvoidance; // 1: Enable APF, 0: Disable
    uint8_t  enableRadarSweep;   // 1: Enable Pan Servo, 0: Center Servo
    uint8_t  speedModeLimit;     // 0: Precision (30%), 1: Normal (70%), 2: Sport (100%)
    
    uint16_t crc16;           // Validation Checksum
};
#pragma pack(pop)
```

### Signal Loss (FailSafe)
The ESP32 firmware monitors the `timestampMs` of the last valid command packet. If no valid packet is received for $\ge 500\text{ ms}$, the system enters **FailSafe Mode**, engaging active short-braking (`TB6612 IN1=HIGH, IN2=HIGH`) and zeroing motor PWM until the connection is restored.
