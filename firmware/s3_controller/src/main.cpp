#include <WiFi.h>
#include <WiFiUdp.h>
#include "Sensors.hpp"
#include "Types.hpp"
#include "LedController.hpp"
#include "WifiManager.hpp"

// ── LBP v2 Firmware Version ───────────────────────────────────────────────────
#define FW_MAJOR 3
#define FW_MINOR 1
#define FW_PATCH 0

// Global sensor and LED objects
S3Sensors  sensors;
LedController led;

// LBP v2 network stack
WifiManager wm;

VehicleTelemetryPacket telemetry;
VehicleCommandPacket   lastCommand;

unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 50; // 20 Hz

// ── CRC-16-CCITT helper (kept here so telemetry can self-sign before calling wm.sendTelemetry)
static uint16_t crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int j = 0; j < 8; ++j)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.printf("Starting ESP32-S3 Firmware v%d.%d.%d\n", FW_MAJOR, FW_MINOR, FW_PATCH);

    // Initialize Sensors (IR Array, BMP280, Software I2C for VL53L0X)
    sensors.init();

    // Initialize RGB status LED
    led.init();

    // Start LBP v2 network stack (non-blocking)
    wm.begin("Airtel_Node", "air66343");
    wm.setBeaconInfo(BOARD_S3_STD, FW_MAJOR, FW_MINOR, FW_PATCH, 3, "ESP32-S3 STD");
    Serial.println("LBP v2 stack started (non-blocking).");

    // Initialize static telemetry fields
    memset(&telemetry, 0, sizeof(VehicleTelemetryPacket));
    telemetry.preamble    = LBP_PREAMBLE_TEL;
    telemetry.hardwareRev = 3; // S3
    telemetry.fwMajor     = FW_MAJOR;
    telemetry.fwMinor     = FW_MINOR;
    telemetry.fwPatch     = FW_PATCH;
    telemetry.boardType   = BOARD_S3_STD;

    Serial.println("Setup complete.");
}

void loop() {
    // 1. Drive LBP v2 network stack (WiFi watchdog + beacon + CMD receive)
    wm.update();

    // 2. Read latest command (WifiManager validated preamble + CRC)
    wm.readCommand(lastCommand);

    // 3. Drive LED based on current command and link state
    led.update(lastCommand, telemetry.statusFlags, wm.hasActiveClient(), millis());

    // 4. Send Telemetry at 20 Hz
    if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL) {
        lastTelemetryTime = millis();

        telemetry.timestampMs  = millis();
        telemetry.irArrayState = sensors.readIRArray();
        telemetry.baroTempC    = sensors.getTemperature();
        telemetry.baroPressurePa = sensors.getPressure();
        telemetry.tof1DistMm   = sensors.getLeftDistanceMm();
        telemetry.tof2DistMm   = sensors.getRightDistanceMm();

        if (sensors.imuOk) {
            float ax, ay, az, gx, gy, gz, t;
            sensors.imu.readScaled(ax, ay, az, gx, gy, gz, t);
            telemetry.linearAccX = ax;
            telemetry.linearAccY = ay;
            telemetry.linearAccZ = az;
            telemetry.imuTempC   = t;

            float roll, pitch;
            sensors.imu.getTilt(roll, pitch);
            telemetry.rollDeg  = roll;
            telemetry.pitchDeg = pitch;
        }

        if (sensors.compassOk) {
            telemetry.headingCompassDeg = sensors.compass.getHeading();
        }

        // Sign and send
        size_t dataLen = sizeof(VehicleTelemetryPacket) - sizeof(uint16_t);
        telemetry.crc16 = crc16(reinterpret_cast<const uint8_t*>(&telemetry), dataLen);
        wm.sendTelemetry(telemetry);
    }
}
