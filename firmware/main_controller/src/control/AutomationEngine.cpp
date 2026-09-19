#include "AutomationEngine.hpp"
#include "TB6612_Driver.hpp"
#include "DualVL53L0X.hpp"
#include <cmath>

#define AEB_THRESHOLD_MM 200 // Automatic Emergency Braking threshold

AutomationEngine::AutomationEngine(TB6612_Driver* driver, DualVL53L0X* radar) 
    : motorDriver(driver), tofRadar(radar), aebEnabled(false), apfEnabled(false) {}

void AutomationEngine::setAEB(bool enable) {
    aebEnabled = enable;
}

void AutomationEngine::setAPF(bool enable) {
    apfEnabled = enable;
}

#define APF_THRESHOLD_MM 600
#define APF_GAIN 1.5f

void AutomationEngine::setAutoTurn(bool enable, int16_t heading) {
    autoTurnEnabled = enable;
    targetHeading = heading;
}

void AutomationEngine::update(int16_t &leftPwm, int16_t &rightPwm, float currentHeading) {
    if (autoTurnEnabled) {
        // ------------------------------------------------------------------
        // Proportional heading controller — calm, capped, no ramp-up.
        //
        // Shortest angular error:
        float diff = targetHeading - currentHeading;
        while (diff >  180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;

        // Dead-band: if within 4° we consider ourselves aligned and stop.
        if (fabsf(diff) <= 4.0f) {
            leftPwm  = 0;
            rightPwm = 0;
            motorDriver->setMotorLeft(0);
            motorDriver->setMotorRight(0);
            return;
        }

        // P-gain: scale error (max 180°) to output range [MIN_TURN..MAX_TURN].
        // At 180° error  -> MAX_TURN (512 PWM, 50% of 1023)
        // At   4° error  -> MIN_TURN (358 PWM, 35% of 1023)
        const int MIN_TURN = 358;
        const int MAX_TURN = 512;

        float scale = fabsf(diff) / 180.0f;   // 0.0 – 1.0
        int   speed = (int)(MIN_TURN + scale * (MAX_TURN - MIN_TURN));
        if (speed > MAX_TURN) speed = MAX_TURN;
        if (speed < MIN_TURN) speed = MIN_TURN;

        // Direction: positive diff means target is clockwise from current heading.
        // Clockwise spin: left motor FORWARD, right motor BACK.
        int steerCmd = (diff > 0) ? speed : -speed;
        leftPwm  =  steerCmd;
        rightPwm = -steerCmd;

        // Push directly to motors — skip APF/AEB while turning
        motorDriver->setMotorLeft(leftPwm);
        motorDriver->setMotorRight(rightPwm);
        return;
    }
    
    uint16_t leftDist = tofRadar->getLeftDistanceMm();
    uint16_t rightDist = tofRadar->getRightDistanceMm();
    
    // Ensure we only use valid readings (ignore crosstalk < 30mm)
    if (leftDist < 30 || leftDist > 8000) leftDist = 8000;
    if (rightDist < 30 || rightDist > 8000) rightDist = 8000;

    if (apfEnabled) {
        // Artificial Potential Field collision avoidance logic
        float repulseLeft = 0.0f;
        float repulseRight = 0.0f;
        
        // If driving forward
        if (leftPwm > 0 || rightPwm > 0) {
            if (leftDist < APF_THRESHOLD_MM) {
                repulseLeft = (APF_THRESHOLD_MM - leftDist) * APF_GAIN;
            }
            if (rightDist < APF_THRESHOLD_MM) {
                repulseRight = (APF_THRESHOLD_MM - rightDist) * APF_GAIN;
            }
            
            // Obstacle on left pushes us right (left+, right-)
            // Obstacle on right pushes us left (left-, right+)
            leftPwm += (int16_t)(repulseLeft - repulseRight);
            rightPwm -= (int16_t)(repulseLeft - repulseRight);
        }
    }

    if (aebEnabled) {
        // If either sensor detects an obstacle closer than the critical threshold, trigger AEB
        if (leftDist < AEB_THRESHOLD_MM || rightDist < AEB_THRESHOLD_MM) {
            // Only brake if trying to drive forward into the obstacle (net forward motion)
            if ((leftPwm + rightPwm) > 0) {
                leftPwm = 0;
                rightPwm = 0;
            }
        }
    }
    
    // Safety check complete, constrain to physical limits before pushing
    if (leftPwm > 1023) leftPwm = 1023;
    if (leftPwm < -1023) leftPwm = -1023;
    if (rightPwm > 1023) rightPwm = 1023;
    if (rightPwm < -1023) rightPwm = -1023;
    
    motorDriver->setMotorLeft(leftPwm);
    motorDriver->setMotorRight(rightPwm);
}
