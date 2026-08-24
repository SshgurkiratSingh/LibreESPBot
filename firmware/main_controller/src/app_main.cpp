#include <Arduino.h>

#ifndef MODE_CALIBRATION
void setup() {
    Serial.begin(115200);
    Serial.println("Starting Rover Main Profile");
}

void loop() {
    delay(1000);
}
#endif
