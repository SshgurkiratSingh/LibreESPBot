#pragma once
#include <cstdint>

// =============================================================================
// LibreBot Protocol v2 (LBP2)
// Version this whenever ANY struct layout changes so firmware/app can detect mismatch.
// =============================================================================
constexpr uint8_t  LBP_PROTOCOL_VERSION = 2;

// Preamble magic words (first 2 bytes of every packet)
constexpr uint16_t LBP_PREAMBLE_TEL  = 0xAA55; // Telemetry: rover -> app
constexpr uint16_t LBP_PREAMBLE_CMD  = 0x55AA; // Command:   app   -> rover
constexpr uint16_t LBP_PREAMBLE_BCN  = 0xBEAC; // Beacon:    rover -> broadcast
constexpr uint16_t LBP_PREAMBLE_PING = 0x9191; // Ping:      app   -> rover
constexpr uint16_t LBP_PREAMBLE_PONG = 0x1919; // Pong:      rover -> app

// Board type IDs
constexpr uint8_t BOARD_UNKNOWN   = 0x00;
constexpr uint8_t BOARD_ROVER_V2  = 0x02; // main_controller (ESP32-WROOM)
constexpr uint8_t BOARD_S3_STD    = 0x03; // s3_controller   (ESP32-S3)

// Network ports
constexpr uint16_t LBP_PORT_CMD    = 8888; // app -> rover (commands + pings)
constexpr uint16_t LBP_PORT_TEL    = 8889; // rover -> app (telemetry + pongs)
constexpr uint16_t LBP_PORT_BEACON = 4210; // rover -> broadcast (beacon)

#pragma pack(push, 1)

// =============================================================================
// LbpBeaconPacket  (approx 34 bytes)
// Sent by rover as UDP broadcast to 255.255.255.255:4210 at 1 Hz.
// App's DiscoveryWorker listens on port 4210 and populates NodeRegistry.
// =============================================================================
struct LbpBeaconPacket {
    uint16_t preamble;         // LBP_PREAMBLE_BCN
    uint8_t  protocolVersion;  // LBP_PROTOCOL_VERSION
    uint8_t  boardType;        // BOARD_ROVER_V2 / BOARD_S3_STD
    uint8_t  fwMajor;
    uint8_t  fwMinor;
    uint8_t  fwPatch;
    uint8_t  hardwareRev;      // PCB revision number
    uint16_t telemetrySize;    // sizeof(VehicleTelemetryPacket) -- mismatch guard
    uint16_t commandSize;      // sizeof(VehicleCommandPacket)
    char     boardName[16];    // e.g. "ESP32-S3 STD" or "Rover V2 STD"
    uint32_t uptimeMs;         // Rover uptime for debug / session ordering
    uint16_t crc16;
};

// =============================================================================
// LbpPingPacket  (10 bytes)
// Sent by app -> rover every 1 s on port 8888.
// Rover echoes back a LbpPongPacket with same seqId + clientMs.
// =============================================================================
struct LbpPingPacket {
    uint16_t preamble;  // LBP_PREAMBLE_PING
    uint16_t seqId;     // Echoed in pong so app can calculate RTT
    uint32_t clientMs;  // App timestamp (ms) -- rover echoes unchanged
    uint16_t crc16;
};

// =============================================================================
// LbpPongPacket  (16 bytes)
// Sent by rover -> app in reply to a ping.
// =============================================================================
struct LbpPongPacket {
    uint16_t preamble;        // LBP_PREAMBLE_PONG
    uint16_t seqId;           // Echoed from ping
    uint32_t clientMs;        // Echoed from ping (for RTT calculation)
    uint32_t roverUptimeMs;   // Rover uptime at time of reply
    float    batteryVoltage;  // Quick battery peek
    uint16_t crc16;
};

// =============================================================================
// VehicleTelemetryPacket
// Sent rover -> app at 20 Hz on port 8889.
// fwMajor/Minor/Patch/boardType embedded so app always knows what board it sees.
// =============================================================================
struct VehicleTelemetryPacket {
    uint16_t preamble;        // LBP_PREAMBLE_TEL (0xAA55)
    uint8_t  hardwareRev;     // PCB revision
    uint8_t  activeImuType;   // 0x01=MPU6050, 0x02=BMI160
    uint8_t  activeMagType;   // 0x01=QMC5883L, 0x02=HMC5883L, 0x03=LIS3MDL
    uint32_t timestampMs;     // Rover uptime (ms)

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
    uint16_t tof1DistMm;      // Left ToF sensor
    uint16_t tof2DistMm;      // Right ToF sensor

    // Actuation & Power
    int16_t  motorLeftPwm;    // -1023 to +1023
    int16_t  motorRightPwm;   // -1023 to +1023
    float    batteryVoltage;
    float    imuTempC;

    // Environment (S3-specific; zeroed on boards without these sensors)
    float    baroTempC;
    float    baroPressurePa;
    uint8_t  irArrayState;    // 7 bits used

    // Status Flags: bit0=Obstacle, bit1=Braking, bit2=RadarActive, bit3=FailSafe
    uint16_t statusFlags;

    // LBP v2 -- version & board identity embedded in every frame
    uint8_t  fwMajor;
    uint8_t  fwMinor;
    uint8_t  fwPatch;
    uint8_t  boardType;       // BOARD_ROVER_V2 / BOARD_S3_STD

    uint16_t crc16;           // CRC-16-CCITT over all bytes except last 2
};

// =============================================================================
// VehicleCommandPacket
// Sent app -> rover at 50 Hz on port 8888.
// =============================================================================
struct VehicleCommandPacket {
    uint16_t preamble;         // LBP_PREAMBLE_CMD (0x55AA)
    uint16_t sequenceId;
    uint8_t  enableAutoTurn;   // 1 = hardware closed-loop turn to heading
    uint8_t  _pad0;            // alignment padding
    int16_t  targetHeading;    // 0-359 degrees
    int16_t  throttleAxis;     // -1023 to +1023
    int16_t  steeringAxis;     // -1023 to +1023

    uint8_t  enableAutoBrake;
    uint8_t  enableApfAvoidance;
    uint8_t  enableRadarSweep;
    uint8_t  radarSweepSpeed;
    uint8_t  speedModeLimit;   // 0=Precision(30%), 1=Normal(70%), 2=Sport(100%)
    uint8_t  headlightMode;    // 0=Off,1=White,2=Police,3=Custom,4=Rainbow,5=Cylon,6=Pattern
    uint8_t  customLedR;
    uint8_t  customLedG;
    uint8_t  customLedB;
    uint8_t  customLedPattern; // 8-bit blinking sequence
    uint8_t  enableNoLagMode;

    uint16_t crc16;
};

#pragma pack(pop)
