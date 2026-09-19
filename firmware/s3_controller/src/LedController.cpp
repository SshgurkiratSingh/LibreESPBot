#include "LedController.hpp"

void LedController::init()
{
    FastLED.addLeds<WS2812, LED_DATA_PIN_1, GRB>(leds1_, LED_NUM_LEDS);
    FastLED.addLeds<WS2812, LED_DATA_PIN_2, GRB>(leds2_, LED_NUM_LEDS);

    // The core fix for the blinding LED: clamp brightness and power so no
    // effect below can exceed these limits.
    FastLED.setBrightness(LED_MAX_BRIGHTNESS);
    FastLED.setMaxPowerInVoltsAndMilliamps(LED_MAX_VOLTS, LED_MAX_MILLIAMPS);

    FastLED.clear();
    FastLED.show();
    Serial.println("[LED] Initialised: brightness capped, not blinding.");
}

void LedController::setPixel(CRGB color)
{
    fill_solid(leds1_, LED_NUM_LEDS, color);
    fill_solid(leds2_, LED_NUM_LEDS, color);
    FastLED.show();
}

void LedController::update(const VehicleCommandPacket& cmd,
                           uint16_t statusFlags,
                           bool linkActive,
                           unsigned long nowMs)
{
    // 1. Safety condition always wins.
    if (statusFlags & 0x08) // Bit 3: FailSafe
    {
        // Rapid red flash.
        setPixel(((nowMs / 150) & 1U) ? CRGB::Red : CRGB::Black);
        return;
    }

    // 2. Explicit app-driven light request overrides the status lamp.
    if (cmd.headlightMode != 0)
    {
        applyHeadlight(cmd, nowMs);
        return;
    }

    // 3. Default behaviour: a dim status lamp.
    applyStatusLamp(linkActive, nowMs);
}

void LedController::applyStatusLamp(bool linkActive, unsigned long nowMs)
{
    if (!linkActive)
    {
        // Not yet in contact with the controller: slow red heartbeat (dim).
        setPixel(((nowMs / 400) & 1U) ? CRGB(255, 0, 0) : CRGB(0, 0, 0));
        return;
    }

    // Link alive: steady dim green with a brief flash each second as a
    // telemetry heartbeat.
    if ((nowMs % 500) < 25)
    {
        setPixel(CRGB(0, 128, 0));
    }
    else
    {
        setPixel(CRGB(0, 24, 0));
    }
}

void LedController::applyHeadlight(const VehicleCommandPacket& cmd, unsigned long nowMs)
{
    switch (cmd.headlightMode)
    {
    case 1: // Headlight ON - warm white, still dimmed.
        setPixel(CRGB(255, 255, 230));
        break;

    case 2: // Police strobe - alternate red / blue.
        switch ((nowMs / 120) % 3)
        {
        case 0: setPixel(CRGB::Red);   break;
        case 1: setPixel(CRGB::Blue);  break;
        default: setPixel(CRGB::Black); break;
        }
        break;

    case 3: // Custom RGB + custom 8-bit blink pattern.
    {
        const uint8_t pattern = cmd.customLedPattern;
        const bool lit = (pattern == 0) || ((pattern >> (nowMs / 200 % 8)) & 1U);
        if (lit)
        {
            setPixel(CRGB(cmd.customLedR, cmd.customLedG, cmd.customLedB));
        }
        else
        {
            setPixel(CRGB::Black);
        }
        break;
    }

    default: // Off / unknown -> black.
        setPixel(CRGB::Black);
        break;
    }
}