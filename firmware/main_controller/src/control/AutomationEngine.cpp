#include "AutomationEngine.hpp"
#include "TB6612_Driver.hpp"
#include "DualVL53L0X.hpp"

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
        float diff = targetHeading - currentHeading;
        while (diff > 180.0f) diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;
        
        if (abs(diff) <= 5.0f) {
            leftPwm = 0;
            rightPwm = 0;
            turnSpeed = 350;
        } else {
            float headingDelta = abs(currentHeading - lastHeading);
            if (headingDelta < 0.5f) {
                stuckTicks++;
                if (stuckTicks >= 25) { // 500ms @ 50Hz
                    turnSpeed += 100;
                    if (turnSpeed > 900) turnSpeed = 900;
                    stuckTicks = 0;
                }
            } else {
                stuckTicks = 0;
                turnSpeed -= 25;
                if (turnSpeed < 350) turnSpeed = 350;
            }
            
            int steerCmd = (diff > 0) ? -turnSpeed : turnSpeed;
            leftPwm = steerCmd;
            rightPwm = -steerCmd;
        }
        lastHeading = currentHeading;
        
        // Skip APF/AEB while auto turning, just push command
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
