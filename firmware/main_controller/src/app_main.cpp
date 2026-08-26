#include <Arduino.h>

#ifndef MODE_CALIBRATION

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include "Types.hpp"

// Hardware Drivers
#include "TB6612_Driver.hpp"
#include "MpuDriver.hpp"
#include "CompassDriver.hpp"
#include "DualVL53L0X.hpp"
#include "AutomationEngine.hpp"
#include "MahonyAHRS.hpp"

// ============================================================
// Global Objects
// ============================================================
TB6612_Driver     motors; // Pins are hardcoded inside TB6612_Driver
MpuDriver         imu;
CompassDriver     compass;
DualVL53L0X       tofSensors;
AutomationEngine  autoEngine(&motors, &tofSensors);
MahonyAHRS        ahrs;

// Sensor availability flags (zeroed readings on failure)
bool imuOk     = false;
bool compassOk = false;
bool tofOk     = false;

WiFiUDP udp;
const uint16_t UDP_PORT = 8888;
IPAddress remoteIP;
uint16_t remotePort = 0;

VehicleTelemetryPacket telemetry;
VehicleCommandPacket   lastCommand;

int16_t lastLeftPwm  = 0; // Last commanded PWM for telemetry
int16_t lastRightPwm = 0;

unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 20; // 50 Hz

unsigned long lastDiscoveryTime = 0;
const unsigned long DISCOVERY_INTERVAL = 1000; // 1 Hz

// ============================================================
// Setup
// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("Starting Rover Main Profile");

    // Initialize I2C
    Wire.begin(21, 22);

    // Initialize Sensors
    imuOk     = imu.begin();
    compassOk = compass.begin();
    tofOk     = tofSensors.init();

    Serial.println("Sensor status:");
    Serial.printf("  MPU6050     : %s\n", imuOk     ? "OK" : "FAILED");
    Serial.printf("  Compass     : %s\n", compassOk ? "OK" : "FAILED");
    Serial.printf("  ToF Left    : %s\n", tofSensors.leftAvailable()  ? "OK" : "FAILED");
    Serial.printf("  ToF Right   : %s\n", tofSensors.rightAvailable() ? "OK" : "FAILED");
    if (!imuOk)     Serial.println("  -> IMU readings zeroed, AHRS disabled.");
    if (!compassOk) Serial.println("  -> Compass heading zeroed.");
    if (!tofOk)     Serial.println("  -> ToF distances zeroed.");
    Serial.println("Continuing startup with available sensors.");

    // Initialize Actuators (init() also releases standby / STBY=HIGH)
    motors.init();

    // Connect to WiFi as station
    WiFi.mode(WIFI_STA);
    WiFi.begin("Airtel_Node", "air66343");

    Serial.print("Connecting to WiFi");
    int wifi_retries = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_retries < 40) {
        delay(500);
        Serial.print(".");
        wifi_retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi.");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Setup mDNS for auto-discovery by the controller app
        if (!MDNS.begin("esp32rover")) {
            Serial.println("Error setting up MDNS responder!");
        } else {
            MDNS.addService("roverctrl", "udp", UDP_PORT);
            MDNS.addServiceTxt("roverctrl", "udp", "drv", "TB6612");
            MDNS.addServiceTxt("roverctrl", "udp", "hw", "Rover V2");
            Serial.println("mDNS responder started: _roverctrl._udp");
        }
    } else {
        Serial.println("\nFailed to connect to WiFi.");
    }

    udp.begin(UDP_PORT);
    Serial.println("UDP listener started.");

    // Init telemetry defaults
    memset(&telemetry, 0, sizeof(VehicleTelemetryPacket));
    telemetry.preamble = 0xAA55;
    telemetry.hardwareRev = 2; // V2
}

// ============================================================
// CRC Check Function
// ============================================================
uint16_t calculateCrc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// ============================================================
// Loop
// ============================================================
void loop() {
    // 1. Check for incoming UDP Commands
    int packetSize = udp.parsePacket();
    if (packetSize == sizeof(VehicleCommandPacket)) {
        udp.read((unsigned char*)&lastCommand, sizeof(VehicleCommandPacket));
        
        size_t dataLen = sizeof(VehicleCommandPacket) - sizeof(uint16_t);
        uint16_t calcCrc = calculateCrc16((const uint8_t*)&lastCommand, dataLen);
        
        if (lastCommand.preamble == 0x55AA && lastCommand.crc16 == calcCrc) {
            // Save remote IP for telemetry reply
            remoteIP = udp.remoteIP();
            remotePort = udp.remotePort();

            // Apply command to Automation Engine & Motors
            autoEngine.setAEB(lastCommand.enableAutoBrake);
            autoEngine.setAPF(lastCommand.enableApfAvoidance);

            // Directly drive motors if no AEB intervention (Basic implementation)
            // Note: AutomationEngine::update() would normally override this
            lastLeftPwm  = lastCommand.throttleAxis + lastCommand.steeringAxis;
            lastRightPwm = lastCommand.throttleAxis - lastCommand.steeringAxis;
            motors.setMotorLeft(lastLeftPwm);
            motors.setMotorRight(lastRightPwm);
        }
    }

    // 2. Automation Update
    autoEngine.update();

    // 3. Sensor Fusion & Telemetry (50Hz)
    if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL) {
        lastTelemetryTime = millis();

        // Read Sensors (zero on failure so no garbage is fed downstream)
        float ax = 0.0f, ay = 0.0f, az = 0.0f;
        float gx = 0.0f, gy = 0.0f, gz = 0.0f, t = 0.0f;
        if (imuOk) {
            imu.readScaled(ax, ay, az, gx, gy, gz, t);
        }

        float heading = 0.0f;
        if (compassOk) {
            heading = compass.getHeading();
        }

        // AHRS only runs when the IMU is present
        float pitch = 0.0f, roll = 0.0f, yaw = 0.0f;
        if (imuOk) {
            ahrs.updateIMU(gx, gy, gz, ax, ay, az); // AHRS from gyro+accel only
            ahrs.getEulerAngles(pitch, roll, yaw);
        }

        // Populate Telemetry
        telemetry.timestampMs = millis();
        telemetry.pitchDeg = pitch;
        telemetry.rollDeg = roll;
        telemetry.yawDeg = yaw;
        telemetry.headingCompassDeg = heading;
        telemetry.linearAccX = ax;
        telemetry.linearAccY = ay;
        telemetry.linearAccZ = az;

        // ToF distances (already zeroed internally for a failed/unavailable sensor)
        telemetry.tof1DistMm = tofSensors.getLeftDistanceMm();
        telemetry.tof2DistMm = tofSensors.getRightDistanceMm();

        telemetry.motorLeftPwm = lastLeftPwm;
        telemetry.motorRightPwm = lastRightPwm;
        telemetry.batteryVoltage = 12.0f; // Mock battery voltage
        
        // Calculate CRC
        size_t tDataLen = sizeof(VehicleTelemetryPacket) - sizeof(uint16_t);
        telemetry.crc16 = calculateCrc16((const uint8_t*)&telemetry, tDataLen);

        if (remotePort != 0) {
            udp.beginPacket(remoteIP, 8889); // Ground Station telemetry listener port
            udp.write((const uint8_t*)&telemetry, sizeof(VehicleTelemetryPacket));
            udp.endPacket();
        }
    }

    // 4. Discovery Beacon (1Hz)
    // The Qt app is passively listening on 5353 for a packet containing _roverctrl._udp.local and drv=
    if (millis() - lastDiscoveryTime >= DISCOVERY_INTERVAL) {
        lastDiscoveryTime = millis();
        udp.beginPacket(IPAddress(224, 0, 0, 251), 5353);
        const char* beacon = "_roverctrl._udp.local\0drv=TB6612\0hw=Rover V2";
        udp.write((const uint8_t*)beacon, 44);
        udp.endPacket();
    }
}

#endif
