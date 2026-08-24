# [Archived] ESP8266 Wi-Fi RC Car

> [!NOTE]  
> **Status: Archived**  
> This directory contains the legacy implementation of the Wi-Fi controlled RC Car powered by an **ESP8266 (NodeMCU)** microcontroller and controlled via an Android mobile application built using MIT App Inventor.

---

## Legacy Overview

The ESP8266 Wi-Fi Car operates as an Access Point (SoftAP). An Android mobile phone connects to the car's Wi-Fi network and sends HTTP GET requests containing control parameters (`State`) to drive the motors, toggle lights, and sound the horn.

---

## Hardware & Pin Configuration

| Component | ESP8266 Pin | GPIO Pin | Function Description |
|---|---|---|---|
| **L298N IN1** | D8 | GPIO 15 | Right Motor Control 1 |
| **L298N IN2** | D7 | GPIO 13 | Right Motor Control 2 |
| **L298N IN3** | D4 | GPIO 2 | Left Motor Control 1 |
| **L298N IN4** | D3 | GPIO 0 | Left Motor Control 2 |
| **Headlight LED** | D2 | GPIO 4 | Front Headlight LED |
| **Buzzer / Horn** | D1 | GPIO 5 | Horn Sound |
| **Brake Light LED** | D5 | GPIO 14 | Rear Red Light |

---

## Wi-Fi & Control Protocol

- **Default SSID:** `Nodemcu_Car`
- **Default Password:** `myespcar`
- **Web Server Port:** `80`

### HTTP Command Protocol (`/ ?State=<COMMAND>`)

| Command Key | Action | Code Function |
|---|---|---|
| `F` | Drive Forward | `goAhead()` |
| `B` | Drive Backward | `goBack()` |
| `L` | Turn Left | `goLeft()` |
| `R` | Turn Right | `goRight()` |
| `I` | Forward Right | `goAheadRight()` |
| `G` | Forward Left | `goAheadLeft()` |
| `J` | Backward Right | `goBackRight()` |
| `H` | Backward Left | `goBackLeft()` |
| `S` | Stop All Motors | `stopRobot()` |
| `W` / `5` | Turn Headlight ON | `digitalWrite(led, HIGH)` |
| `0` | Turn Headlight OFF | `digitalWrite(led, LOW)` |
| `V` | Toggle Horn/Buzzer | `digitalWrite(bzr, !led_state)` |

---

## Android Application & Source Files

- **`Esp8266-Car.apk`**: Pre-built Android package ready to be installed on Android devices.
- **`Customisation Assets/esp_car.aia`**: MIT App Inventor project file allowing user customization of control UI and logic.
- **`Customisation Assets/Icons/`**: Icon assets used in the application UI.

---

## File Structure

```text
archive/esp8266/
├── Customisation Assets/
│   ├── Icons/          # UI Icon PNG assets
│   └── esp_car.aia     # MIT App Inventor source project
├── Esp8266-Car.apk     # Compiled Android installer
├── esp-car.ino         # Legacy Arduino firmware for ESP8266
└── README.md           # Documentation for archived build
```
