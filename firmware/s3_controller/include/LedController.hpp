#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include "Types.hpp"

// ==========================================
// Onboard RGB LED (WS2812 addressable)
// ESP32-S3-DevKitC-1 -> GPIO48
// ==========================================
#define LED_DATA_PIN 48
#define LED_NUM_LEDS 1

// The onboard RGB LED is bright enough to be blinding. Cap the global
// brightness and the theoretical power draw so it can *never* run full-blast,
// regardless of what the controller app requests.
#define LED_MAX_BRIGHTNESS 32   // ~12.5% of 255 (was blinding at full scale)
#define LED_MAX_MILLIAMPS  50   // cap theoretical current on 5V rail
#define LED_MAX_VOLTS       5

class LedController
{
public:
    void init();

    // Called once per loop tick (~20 Hz). Picks the colour/effect based on
    // system state and the controller's light commands, then shows the LED.
    //   cmd        - most recently received command frame (headlights/custom LED)
    //   statusFlags- telemetry status flags (Bit 3 = FailSafe)
    //   linkActive - true once a command has been received from the controller
    //   nowMs      - uptime in ms for blink/timing logic
    void update(const VehicleCommandPacket& cmd, uint16_t statusFlags,
                bool linkActive, unsigned long nowMs);

private:
    void setPixel(CRGB color);                 // write + show (brightness stays capped)
    void applyHeadlight(const VehicleCommandPacket& cmd, unsigned long nowMs);
    void applyStatusLamp(bool linkActive, unsigned long nowMs);

    CRGB leds_[LED_NUM_LEDS];
};