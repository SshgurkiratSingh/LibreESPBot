#include <Arduino.h>

#ifdef MODE_CALIBRATION

#include "TB6612_Driver.hpp"
#include "MpuDriver.hpp"
#include "CompassDriver.hpp"
#include "DualVL53L0X.hpp"
#include <ESP32Servo.h>

TB6612_Driver motors;
MpuDriver imu;
CompassDriver compass;
DualVL53L0X tofSensors;
Servo panServo;

#define SERVO_PIN 18 

unsigned long lastSensorRead = 0;
unsigned long lastMotorAction = 0;
int motorState = 0;
int servoAngle = 0;
int servoDir = 1;

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- ESP32 Hardware Test Mode ---");

    Wire.begin(21, 22);

    // Init Sensors
    if (imu.begin()) Serial.println("MPU-6050 OK"); else Serial.println("MPU-6050 FAILED");
    if (compass.begin()) Serial.println("Compass OK"); else Serial.println("Compass FAILED");
    if (tofSensors.init()) Serial.println("Dual ToF OK"); else Serial.println("Dual ToF FAILED");

    // Init Actuators
    motors.init();
    
    // Setup Servo
    ESP32PWM::allocateTimer(2);
    panServo.setPeriodHertz(50);
    panServo.attach(SERVO_PIN, 500, 2400);
    Serial.println("Servo attached to Pin 18. Beginning sweep test...");
}

void loop() {
    // 1. Read Sensors periodically (every 500ms)
    if (millis() - lastSensorRead > 500) {
        lastSensorRead = millis();
        
        float ax, ay, az, gx, gy, gz, temp;
        imu.readScaled(ax, ay, az, gx, gy, gz, temp);
        
        float heading = compass.getHeading();
        
        uint16_t distL = tofSensors.getLeftDistanceMm();
        uint16_t distR = tofSensors.getRightDistanceMm();
        
        Serial.printf("[SENSORS] Acc: %.2f %.2f %.2f | Gyro: %.2f %.2f %.2f | Head: %.1f | ToF: L=%dmm R=%dmm\n",
                      ax, ay, az, gx, gy, gz, heading, distL, distR);
                      
        // Sweep Servo
        servoAngle += servoDir * 15;
        if (servoAngle >= 180) { servoAngle = 180; servoDir = -1; }
        if (servoAngle <= 0) { servoAngle = 0; servoDir = 1; }
        panServo.write(servoAngle);
    }
    
    // 2. Cycle Motors periodically (every 2 seconds)
    if (millis() - lastMotorAction > 2000) {
        lastMotorAction = millis();
        
        switch (motorState) {
            case 0:
                Serial.println("[MOTORS] Forward (400)");
                motors.setMotorLeft(400);
                motors.setMotorRight(400);
                motorState = 1;
                break;
            case 1:
                Serial.println("[MOTORS] Brake");
                motors.setMotorLeft(0);
                motors.setMotorRight(0);
                motorState = 2;
                break;
            case 2:
                Serial.println("[MOTORS] Reverse (-400)");
                motors.setMotorLeft(-400);
                motors.setMotorRight(-400);
                motorState = 3;
                break;
            case 3:
                Serial.println("[MOTORS] Brake");
                motors.setMotorLeft(0);
                motors.setMotorRight(0);
                motorState = 0;
                break;
        }
    }
}

#endif
