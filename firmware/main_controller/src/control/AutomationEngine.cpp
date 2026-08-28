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

void AutomationEngine::update(int16_t &leftPwm, int16_t &rightPwm) {
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
            
            // Constrain to physical limits
            if (leftPwm > 1023) leftPwm = 1023;
            if (leftPwm < -1023) leftPwm = -1023;
            if (rightPwm > 1023) rightPwm = 1023;
            if (rightPwm < -1023) rightPwm = -1023;
        }
    }

    if (aebEnabled) {
        // If either sensor detects an obstacle closer than the critical threshold, trigger AEB
        if (leftDist < AEB_THRESHOLD_MM || rightDist < AEB_THRESHOLD_MM) {
            // Only brake if trying to drive forward into the obstacle
            if (leftPwm > 0 || rightPwm > 0) {
                leftPwm = 0;
                rightPwm = 0;
            }
        }
    }
    
    // Safety check complete, push final safe command to hardware
    motorDriver->setMotorLeft(leftPwm);
    motorDriver->setMotorRight(rightPwm);
}
