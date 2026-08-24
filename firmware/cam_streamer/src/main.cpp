#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-CAM Streamer Node Initializing...");
    // TODO: Initialize OV2640 and RTSP Stream
}

void loop() {
    // Keep FreeRTOS idle task happy
    delay(100);
}
