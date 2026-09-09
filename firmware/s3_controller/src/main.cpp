#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include "Sensors.hpp"
#include "Types.hpp"

// Global Sensor Object
S3Sensors sensors;

WiFiUDP udp;
const uint16_t UDP_PORT = 8888;
IPAddress remoteIP;
uint16_t remotePort = 0;

VehicleTelemetryPacket telemetry;
VehicleCommandPacket lastCommand;

unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 50; // 20 Hz

unsigned long lastDiscoveryTime = 0;
const unsigned long DISCOVERY_INTERVAL = 1000; // 1 Hz

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

void setup() {
    Serial.begin(115200);
    delay(1000); // Give serial monitor time to open
    
    Serial.println("Starting ESP32-S3 Custom Firmware...");
    
    // Initialize Sensors (IR Array, BMP280, Software I2C for VL53L0X)
    sensors.init();
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin("Airtel_Node", "air66343");

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
        WiFi.setSleep(false);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        if (!MDNS.begin("esp32s3rover"))
        {
            Serial.println("Error setting up MDNS responder!");
        }
        else
        {
            MDNS.addService("roverctrl", "udp", UDP_PORT);
            MDNS.addServiceTxt("roverctrl", "udp", "drv", "TB6612");
            MDNS.addServiceTxt("roverctrl", "udp", "hw", "Rover V2 S3");
            Serial.println("mDNS responder started: _roverctrl._udp");
        }
    }

    udp.begin(UDP_PORT);
    Serial.println("UDP listener started.");

    memset(&telemetry, 0, sizeof(VehicleTelemetryPacket));
    telemetry.preamble = 0xAA55;
    telemetry.hardwareRev = 3; // V3 for S3
    
    Serial.println("Setup complete.");
}

void loop() {
    // 1. Handle incoming UDP commands
    int packetSize = udp.parsePacket();
    while (packetSize > 0)
    {
        if (packetSize == sizeof(VehicleCommandPacket)) {
            udp.read((unsigned char *)&lastCommand, sizeof(VehicleCommandPacket));
            
            size_t dataLen = sizeof(VehicleCommandPacket) - sizeof(uint16_t);
            uint16_t calcCrc = calculateCrc16((const uint8_t *)&lastCommand, dataLen);

            if (lastCommand.preamble == 0x55AA && lastCommand.crc16 == calcCrc)
            {
                remoteIP = udp.remoteIP();
                remotePort = udp.remotePort();
            }
        } else {
            udp.flush();
        }
        packetSize = udp.parsePacket();
    }
    
    // 2. Send Telemetry
    if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL)
    {
        lastTelemetryTime = millis();
        
        telemetry.timestampMs = millis();
        telemetry.irArrayState = sensors.readIRArray();
        telemetry.baroTempC = sensors.getTemperature();
        telemetry.baroPressurePa = sensors.getPressure();
        telemetry.tof1DistMm = sensors.getLeftDistanceMm();
        telemetry.tof2DistMm = sensors.getRightDistanceMm();
        
        if (sensors.imuOk) {
            float ax, ay, az, gx, gy, gz, t;
            sensors.imu.readScaled(ax, ay, az, gx, gy, gz, t);
            telemetry.linearAccX = ax;
            telemetry.linearAccY = ay;
            telemetry.linearAccZ = az;
            telemetry.imuTempC = t;
            
            float roll, pitch;
            sensors.imu.getTilt(roll, pitch);
            telemetry.rollDeg = roll;
            telemetry.pitchDeg = pitch;
        }
        
        if (sensors.compassOk) {
            telemetry.headingCompassDeg = sensors.compass.getHeading();
        }
        
        size_t tDataLen = sizeof(VehicleTelemetryPacket) - sizeof(uint16_t);
        telemetry.crc16 = calculateCrc16((const uint8_t *)&telemetry, tDataLen);

        if (remotePort != 0)
        {
            udp.beginPacket(remoteIP, 8889);
            udp.write((const uint8_t *)&telemetry, sizeof(VehicleTelemetryPacket));
            udp.endPacket();
        }
    }
    
    // 3. Discovery Beacon (1Hz)
    if (millis() - lastDiscoveryTime >= DISCOVERY_INTERVAL)
    {
        lastDiscoveryTime = millis();
        udp.beginPacket(IPAddress(224, 0, 0, 251), 5353);
        const char *beacon = "_roverctrl._udp.local\0drv=TB6612\0hw=Rover V2 S3";
        udp.write((const uint8_t *)beacon, 47);
        udp.endPacket();
    }
}
