# Servo Non-Working Investigation

I have thoroughly investigated the potential reasons why the radar pan servo (connected to GPIO 18) may not be functioning correctly. Below is a breakdown of the likely hardware and software causes, and the fixes applied.

## 1. Software / Timer Conflicts (Resolved)
**The Problem:** The ESP32's internal LEDC (LED Control) peripheral has 16 channels and 8 timers (4 High-Speed, 4 Low-Speed). The motor driver (`TB6612_Driver`) uses High-Speed channels 6 and 7 for 20kHz PWM. If the native `ledcSetup()` was used for the servo at 50Hz on a shared timer, the ESP32's APB clock limits would cause the frequency to be incorrectly generated, resulting in a dead servo.
**The Fix:** I have restored the `madhephaestus/ESP32Servo` library, which safely abstracts this by dynamically allocating a free, non-conflicting timer (Timer 2) and correctly configuring the 1MHz `REF_TICK` clock to achieve a precise 50Hz signal. 

## 2. Default UI State (Check this first!)
**The Problem:** In the firmware, if `lastCommand.enableRadarSweep` is false, the servo locks itself at exactly 90 degrees and stops sweeping.
**The Fix:** Ensure that you have **toggled the "Radar Sweep [R]" switch in the LibreESPBot app to the ON position**. Additionally, make sure "NoLag Hard Realtime" mode is **OFF**, as NoLag mode explicitly suspends the servo task to save CPU cycles.

## 3. Hardware / Power Delivery (Likely Culprit)
If the software fixes above and toggling the UI switch do not bring the servo back to life, the issue is almost certainly electrical:
* **Brownouts:** SG90 / MG996R servos draw significant stall current (upwards of 500mA - 1A). If the servo's VCC is connected directly to the ESP32's `3V3` pin, it will not have enough current/voltage to move, and it may even cause the ESP32 to silently brown-out and reset the sensor loop.
* **Solution:** Ensure the servo is powered from a robust `5V` rail (such as the VIN pin if powered by USB, or directly from the motor battery via a buck converter). 
* **Signal Voltage:** While servos run on 5V power, their PWM signal pin usually accepts 3.3V logic from GPIO 18 just fine. However, double-check that the ground (GND) wire of the servo is shared with the ESP32's GND.

## 4. Pin Configuration
The firmware is strictly configured to output the Servo PWM signal on **GPIO 18**. 
* Verify that the yellow/orange PWM wire of the servo is securely connected to GPIO 18 on your ESP32 dev board.
