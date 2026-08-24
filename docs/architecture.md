# System Architecture

## Overview
The ESP32 Rover Platform is divided into two primary subsystems communicating over a high-speed, zero-copy UDP link over a local Wi-Fi network.

1. **Firmware (ESP32 Node)**: Real-time sensor fusion, hardware abstraction, and kinematic control.
2. **Software (Qt 6 Ground Station)**: Cross-platform user interface, data visualization, and input transmission.

---

## 1. Embedded Firmware Layer (PlatformIO)

The firmware is designed using the Arduino core atop ESP-IDF, leveraging FreeRTOS for task management.

### Hardware Polymorphism
The platform supports a modular hardware approach. Using `platformio.ini` build flags, the codebase can be statically compiled for different sensor loadouts without incurring dynamic execution overhead.
- **IMU Support**: MPU6050, BMI160.
- **Compass Support**: QMC5883L, HMC5883L, LIS3MDL.
- **Motor Drivers**: TB6612FNG (current default), expandable to L298N or others.

### Sensor Fusion & Automation
- **Mahony AHRS**: The 9-DoF (Degrees of Freedom) Mahony filter continuously merges Gyroscope, Accelerometer, and Magnetometer data to provide drift-compensated Euler angles (Pitch, Roll, Yaw).
- **Time-of-Flight (ToF)**: Dual VL53L0X sensors operate on a shared I2C bus utilizing staged `XSHUT` soft-addressing (booting sequentially to assign dynamic I2C addresses 0x30 and 0x31).
- **Automation Engine**: Executes Automatic Emergency Braking (AEB) based on ToF distances to prevent collisions.

---

## 2. Qt 6 Ground Station Layer

The controller application is built on the Qt 6 framework, designed for seamless cross-compilation to Windows, Linux (AppImage), and Android.

### Networking & Discovery
- **mDNS DiscoveryWorker**: The software passively listens on multicast `224.0.0.251:5353` for `_roverctrl._udp.local` services. Once discovered, it extracts the rover's IP and hardware profile (embedded in TXT records).
- **Asynchronous UDP**: 
  - `TelemetryClient` listens on port 8889 for incoming 50Hz telemetry packets.
  - `CommandEmitter` dispatches actuation commands on port 8888 at a deterministic 50Hz timer interval.

### UI Architecture (QML)
- **Virtual Joysticks**: Multi-touch support with an exponential response curve `u_exp(v) = sgn(v) * |v|^1.6` for precise micro-adjustments.
- **Polar Radar Canvas**: Converts the polar coordinates (distance and servo angle) from the ToF sensors into a Cartesian (X,Y) point cloud buffer, rendering dynamic obstacles.
- **Artificial Horizon**: Uses 2D QtQuick transformations to project the real-time Pitch and Roll data from the Mahony AHRS into an aerospace-grade attitude indicator.
