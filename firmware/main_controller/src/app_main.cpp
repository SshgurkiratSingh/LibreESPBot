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
#include <FastLED.h>
#include <ESP32Servo.h>

// ============================================================
// Global Objects
// ============================================================
TB6612_Driver motors; // Pins are hardcoded inside TB6612_Driver
MpuDriver imu;
CompassDriver compass;
DualVL53L0X tofSensors;
Servo panServo;
AutomationEngine autoEngine(&motors, &tofSensors);
MahonyAHRS ahrs;

// Sensor availability flags (zeroed readings on failure)
bool imuOk = false;
bool compassOk = false;
bool tofOk = false;

WiFiUDP udp;
const uint16_t UDP_PORT = 8888;
IPAddress remoteIP;
uint16_t remotePort = 0;

VehicleTelemetryPacket telemetry;
VehicleCommandPacket lastCommand;

int16_t baseLeftPwm = 0; // Last commanded base PWM before APF/AEB
int16_t baseRightPwm = 0;

int servoAngle = 90;
int servoDir = 1;
unsigned long lastServoTime = 0;

// LED Strips
#define NUM_LEDS 3
#define LED_PIN_LEFT 23
#define LED_PIN_RIGHT 19
CRGB ledsLeft[NUM_LEDS];
CRGB ledsRight[NUM_LEDS];

unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 20; // 50 Hz

unsigned long lastDiscoveryTime = 0;
const unsigned long DISCOVERY_INTERVAL = 1000; // 1 Hz

// ============================================================
// Setup
// ============================================================
void setup()
{
    Serial.begin(115200);
    Serial.println("Starting Rover Main Profile");

    // Initialize I2C
    Wire.begin(21, 22);

    // Initialize Sensors
    imuOk = imu.begin();
    compassOk = compass.begin();
    tofOk = tofSensors.init();

    // Battery Voltage Pin
    pinMode(34, INPUT);

    Serial.println("Sensor status:");
    Serial.printf("  MPU6050     : %s\n", imuOk ? "OK" : "FAILED");
    Serial.printf("  Compass     : %s\n", compassOk ? "OK" : "FAILED");
    Serial.printf("  ToF Left    : %s\n", tofSensors.leftAvailable() ? "OK" : "FAILED");
    Serial.printf("  ToF Right   : %s\n", tofSensors.rightAvailable() ? "OK" : "FAILED");
    if (!imuOk)
        Serial.println("  -> IMU readings zeroed, AHRS disabled.");
    if (!compassOk)
        Serial.println("  -> Compass heading zeroed.");
    if (!tofOk)
        Serial.println("  -> ToF distances zeroed.");
    Serial.println("Continuing startup with available sensors.");

    // Initialize Actuators (init() also releases standby / STBY=HIGH)
    motors.init();

    // Initialize Radar Servo
    ESP32PWM::allocateTimer(2);
    panServo.setPeriodHertz(50);
    panServo.attach(18, 500, 2400);
    panServo.write(90);
    Serial.println("Servo attached on GPIO18");

    // Initialize Addressable LEDs
    FastLED.addLeds<WS2812B, LED_PIN_LEFT, GRB>(ledsLeft, NUM_LEDS);
    FastLED.addLeds<WS2812B, LED_PIN_RIGHT, GRB>(ledsRight, NUM_LEDS);
    FastLED.setBrightness(100);
    FastLED.clear(true);

    // Connect to WiFi as station
    WiFi.mode(WIFI_STA);
    WiFi.begin("ConForNode", "12345678");

    Serial.print("Connecting to WiFi");
    int wifi_retries = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_retries < 40)
    {
        delay(500);
        Serial.print(".");
        wifi_retries++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nConnected to WiFi.");
        WiFi.setSleep(false); // Disable Wi-Fi power saving to fix UDP dropouts!
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Setup mDNS for auto-discovery by the controller app
        if (!MDNS.begin("esp32rover"))
        {
            Serial.println("Error setting up MDNS responder!");
        }
        else
        {
            MDNS.addService("roverctrl", "udp", UDP_PORT);
            MDNS.addServiceTxt("roverctrl", "udp", "drv", "TB6612");
            MDNS.addServiceTxt("roverctrl", "udp", "hw", "Rover V2");
            Serial.println("mDNS responder started: _roverctrl._udp");
        }
    }
    else
    {
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
uint16_t calculateCrc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; ++j)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ 0x1021;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// ============================================================
// Loop
// ============================================================
void loop()
{
    // 1. Check for incoming UDP Commands
    bool newCommandReceived = false;
    int packetSize = udp.parsePacket();
    while (packetSize > 0)
    {
        if (packetSize == sizeof(VehicleCommandPacket)) {
            udp.read((unsigned char *)&lastCommand, sizeof(VehicleCommandPacket));
            newCommandReceived = true;
        } else {
            udp.flush(); // Discard invalid packets
        }
        packetSize = udp.parsePacket();
    }

    if (newCommandReceived)
    {
        size_t dataLen = sizeof(VehicleCommandPacket) - sizeof(uint16_t);
        uint16_t calcCrc = calculateCrc16((const uint8_t *)&lastCommand, dataLen);

        if (lastCommand.preamble == 0x55AA && lastCommand.crc16 == calcCrc)
        {
            // Save remote IP for telemetry reply
            remoteIP = udp.remoteIP();
            remotePort = udp.remotePort();

            Serial.printf("Cmd RX | Throttle: %d, Steering: %d, AEB: %d, APF: %d\n",
                          lastCommand.throttleAxis, lastCommand.steeringAxis,
                          lastCommand.enableAutoBrake, lastCommand.enableApfAvoidance);

            // Apply command to Automation Engine (Motors are NOT driven here anymore)
            // They are driven safely inside autoEngine.update()
            autoEngine.setAEB(lastCommand.enableAutoBrake);
            autoEngine.setAPF(lastCommand.enableApfAvoidance);

            float speedMult = 1.0f;
            if (lastCommand.speedModeLimit == 0) speedMult = 0.15f;      // Crawl
            else if (lastCommand.speedModeLimit == 1) speedMult = 0.3f; // Precision
            else if (lastCommand.speedModeLimit == 2) speedMult = 0.7f; // Normal
            else speedMult = 1.0f;                                      // Sport

            baseLeftPwm = (lastCommand.throttleAxis + lastCommand.steeringAxis) * speedMult;
            baseRightPwm = (lastCommand.throttleAxis - lastCommand.steeringAxis) * speedMult;
        }
    }

    // 2. Automation Update (Mixes APF steering & AEB braking)
    int16_t currentLeft = baseLeftPwm;
    int16_t currentRight = baseRightPwm;
    autoEngine.update(currentLeft, currentRight);

    // Radar Sweep Update (Skip if noLagMode)
    if (!lastCommand.enableNoLagMode && lastCommand.enableRadarSweep)
    {
        if (millis() - lastServoTime > 15)
        {
            lastServoTime = millis();
            servoAngle += servoDir * 2;
            if (servoAngle >= 180) { servoAngle = 180; servoDir = -1; }
            if (servoAngle <= 0) { servoAngle = 0; servoDir = 1; }
            panServo.write(servoAngle);
        }
    }
    else if (!lastCommand.enableNoLagMode)
    {
        if (servoAngle != 90)
        {
            servoAngle = 90;
            panServo.write(90);
        }
    }

    // 3. Sensor Fusion & Telemetry (50Hz) (Skip if noLagMode)
    if (!lastCommand.enableNoLagMode && millis() - lastTelemetryTime >= TELEMETRY_INTERVAL)
    {
        lastTelemetryTime = millis();

        // Read Sensors (zero on failure so no garbage is fed downstream)
        float ax = 0.0f, ay = 0.0f, az = 0.0f;
        float gx = 0.0f, gy = 0.0f, gz = 0.0f, t = 0.0f;
        if (imuOk)
        {
            imu.readScaled(ax, ay, az, gx, gy, gz, t);
        }

        float heading = 0.0f;
        if (compassOk)
        {
            heading = compass.getHeading();
        }

        // AHRS only runs when the IMU is present
        float pitch = 0.0f, roll = 0.0f, yaw = 0.0f;
        if (imuOk)
        {
            // Mahony AHRS requires Gyro in Radians per second, but MPU driver outputs Degrees per second
            float gx_rad = gx * PI / 180.0f;
            float gy_rad = gy * PI / 180.0f;
            float gz_rad = gz * PI / 180.0f;

            ahrs.updateIMU(gx_rad, gy_rad, gz_rad, ax, ay, az); // AHRS from gyro+accel only
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

        telemetry.motorLeftPwm = motors.getCurrentLeftPwm();
        telemetry.motorRightPwm = motors.getCurrentRightPwm();

        // 1/3 Voltage Divider on Pin 34
        // ADC 0-4095 maps to 0-3.3V (default 11dB attenuation)
        float v_adc = (analogRead(34) / 4095.0f) * 3.3f;
        telemetry.batteryVoltage = v_adc * 4.3f;

        telemetry.imuTempC = t;
        telemetry.servoAngleDeg = servoAngle - 90; // -90 to +90
        telemetry.statusFlags = 0x00;

        // Update FastLED based on lastCommand
        bool leftBlinker = (lastCommand.steeringAxis < -100);
        bool rightBlinker = (lastCommand.steeringAxis > 100);
        bool reverse = (lastCommand.throttleAxis < -50);
        bool blinkState = (millis() % 1000) > 500;

        uint8_t headlightMode = lastCommand.headlightMode;

        for (int i = 0; i < NUM_LEDS; i++)
        {
            if (reverse)
            {
                ledsLeft[i] = CRGB::Red;
                ledsRight[i] = CRGB::Red;
            }
            else
            {
                if (headlightMode == 1)
                {
                    ledsLeft[i] = CRGB(100, 100, 100); // Bright white
                    ledsRight[i] = CRGB(100, 100, 100);
                }
                else if (headlightMode == 2)
                {
                    // Police Strobe logic
                    bool strobeLeft = (millis() % 400) < 200;
                    bool strobeFast = (millis() % 100) < 50;
                    if (strobeLeft && strobeFast)
                    {
                        ledsLeft[i] = CRGB::Red;
                        ledsRight[i] = CRGB::Black;
                    }
                    else if (!strobeLeft && strobeFast)
                    {
                        ledsLeft[i] = CRGB::Black;
                        ledsRight[i] = CRGB::Blue;
                    }
                    else
                    {
                        ledsLeft[i] = CRGB::Black;
                        ledsRight[i] = CRGB::Black;
                    }
                }
                else if (headlightMode == 3)
                {
                    CRGB customColor = CRGB(lastCommand.customLedR, lastCommand.customLedG, lastCommand.customLedB);
                    ledsLeft[i] = customColor;
                    ledsRight[i] = customColor;
                }
                else if (headlightMode == 4)
                {
                    // Rainbow Mode
                    uint8_t hueOffset = (millis() / 20) % 255;
                    ledsLeft[i] = CHSV(hueOffset + (i * 255 / NUM_LEDS), 255, 255);
                    ledsRight[i] = CHSV(hueOffset + (i * 255 / NUM_LEDS), 255, 255);
                }
                else if (headlightMode == 5)
                {
                    // Cylon Scanner Mode (Sweeping red dot)
                    // Sweeps back and forth across the 3 LEDs
                    int phase = (millis() / 150) % 4; // 0, 1, 2, 1
                    int activeLed = (phase == 3) ? 1 : phase;
                    ledsLeft[i] = (i == activeLed) ? CRGB::Red : CRGB::Black;
                    ledsRight[i] = (i == activeLed) ? CRGB::Red : CRGB::Black;
                }
                else
                {
                    ledsLeft[i] = CRGB::Black; // Off
                    ledsRight[i] = CRGB::Black;
                }
            }
        }

        // Blinkers override all - Audi style sequential
        // 4 phases: 0 (Off), 1 (Inner), 2 (Middle), 3 (Outer)
        int blinkPhase = (millis() % 600) / 150;

        if (leftBlinker)
        {
            for (int i = 0; i < NUM_LEDS; i++)
                ledsLeft[i] = CRGB::Black;
            if (blinkPhase >= 1 && NUM_LEDS > 0)
                ledsLeft[NUM_LEDS - 1] = CRGB(255, 120, 0);
            if (blinkPhase >= 2 && NUM_LEDS > 1)
                ledsLeft[NUM_LEDS - 2] = CRGB(255, 120, 0);
            if (blinkPhase >= 3 && NUM_LEDS > 2)
                ledsLeft[NUM_LEDS - 3] = CRGB(255, 120, 0);
        }
        if (rightBlinker)
        {
            for (int i = 0; i < NUM_LEDS; i++)
                ledsRight[i] = CRGB::Black;
            if (blinkPhase >= 1 && NUM_LEDS > 0)
                ledsRight[NUM_LEDS - 1] = CRGB(255, 120, 0);
            if (blinkPhase >= 2 && NUM_LEDS > 1)
                ledsRight[NUM_LEDS - 2] = CRGB(255, 120, 0);
            if (blinkPhase >= 3 && NUM_LEDS > 2)
                ledsRight[NUM_LEDS - 3] = CRGB(255, 120, 0);
        }
        FastLED.show();

        // Calculate CRC
        size_t tDataLen = sizeof(VehicleTelemetryPacket) - sizeof(uint16_t);
        telemetry.crc16 = calculateCrc16((const uint8_t *)&telemetry, tDataLen);

        if (remotePort != 0)
        {
            udp.beginPacket(remoteIP, 8889); // LibreESPBot telemetry listener port
            udp.write((const uint8_t *)&telemetry, sizeof(VehicleTelemetryPacket));
            udp.endPacket();
        }
    } // End of !lastCommand.enableNoLagMode block for LED & Telemetry

    // 4. Discovery Beacon (1Hz)
    // The Qt app is passively listening on 5353 for a packet containing _roverctrl._udp.local and drv=
    if (millis() - lastDiscoveryTime >= DISCOVERY_INTERVAL)
    {
        lastDiscoveryTime = millis();
        udp.beginPacket(IPAddress(224, 0, 0, 251), 5353);
        const char *beacon = "_roverctrl._udp.local\0drv=TB6612\0hw=Rover V2";
        udp.write((const uint8_t *)beacon, 44);
        udp.endPacket();
    }
}

#endif
