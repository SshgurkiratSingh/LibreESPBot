#include "TB6612_Driver.hpp"
#include <Arduino.h>
#include <cmath>

#define PWMA 27
#define AIN1 12
#define AIN2 13
#define PWMB 33
#define BIN1 26
#define BIN2 25
#define STBY 32 // Assuming 32 since STBY was moved to 33

#define PWM_FREQ 20000
#define PWM_RES 10
#define PWM_DB 150 // Deadband stiction threshold

TB6612_Driver::TB6612_Driver() {}

void TB6612_Driver::init() {
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);
    pinMode(STBY, OUTPUT);

    // Configure ESP32 LEDC channels at 20 kHz with 10-bit resolution
    ledcSetup(0, PWM_FREQ, PWM_RES);
    ledcAttachPin(PWMA, 0);

    ledcSetup(1, PWM_FREQ, PWM_RES);
    ledcAttachPin(PWMB, 1);

    digitalWrite(STBY, HIGH);
}

int16_t TB6612_Driver::applyDeadband(int16_t u) {
    if (u == 0) return 0;
    
    float sign = (u > 0) ? 1.0f : -1.0f;
    float abs_u = std::abs((float)u);
    
    // PWM_applied = sgn(u) * [PWM_db + (1 - PWM_db/1023) * |u|]
    float pwm_applied = sign * (PWM_DB + (1.0f - (float)PWM_DB / 1023.0f) * abs_u);
    return (int16_t)pwm_applied;
}

void TB6612_Driver::setMotorLeft(int16_t speed) {
    // Slew rate limiting to prevent brownout
    if (speed > currentLeft + MAX_SLEW_STEP) currentLeft += MAX_SLEW_STEP;
    else if (speed < currentLeft - MAX_SLEW_STEP) currentLeft -= MAX_SLEW_STEP;
    else currentLeft = speed;
    
    int16_t speed_out = applyDeadband(currentLeft);
    
    if (speed_out > 0) {
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, LOW);
        ledcWrite(0, speed_out);
    } else if (speed_out < 0) {
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, HIGH);
        ledcWrite(0, -speed_out);
    } else {
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, HIGH); // Active short braking
        ledcWrite(0, 0);
    }
}

void TB6612_Driver::setMotorRight(int16_t speed) {
    // Slew rate limiting to prevent brownout
    if (speed > currentRight + MAX_SLEW_STEP) currentRight += MAX_SLEW_STEP;
    else if (speed < currentRight - MAX_SLEW_STEP) currentRight -= MAX_SLEW_STEP;
    else currentRight = speed;
    
    int16_t speed_out = applyDeadband(currentRight);
    
    if (speed_out > 0) {
        digitalWrite(BIN1, HIGH);
        digitalWrite(BIN2, LOW);
        ledcWrite(1, speed_out);
    } else if (speed_out < 0) {
        digitalWrite(BIN1, LOW);
        digitalWrite(BIN2, HIGH);
        ledcWrite(1, -speed_out);
    } else {
        digitalWrite(BIN1, HIGH);
        digitalWrite(BIN2, HIGH); // Active short braking
        ledcWrite(1, 0);
    }
}
