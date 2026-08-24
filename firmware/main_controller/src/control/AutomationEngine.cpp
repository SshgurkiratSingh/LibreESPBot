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

void AutomationEngine::update() {
    if (aebEnabled) {
        uint16_t leftDist = tofRadar->getLeftDistanceMm();
        uint16_t rightDist = tofRadar->getRightDistanceMm();

        // If either sensor detects an obstacle closer than the threshold, trigger AEB
        if ((leftDist > 0 && leftDist < AEB_THRESHOLD_MM) || 
            (rightDist > 0 && rightDist < AEB_THRESHOLD_MM)) {
            
            // Active short-braking
            motorDriver->setMotorLeft(0);
            motorDriver->setMotorRight(0);
        }
    }

    if (apfEnabled) {
        // Artificial Potential Field collision avoidance logic
        // Calculate repulsion vector from detected obstacles and steer away
        // ... (APF algorithm implementation placeholder) ...
    }
}
