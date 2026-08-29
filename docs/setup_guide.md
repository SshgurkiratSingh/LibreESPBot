# Development & Setup Guide

This guide provides step-by-step instructions for building the embedded firmware and compiling the cross-platform controller software.

## 1. Prerequisites

### Embedded Toolchain
- **Python 3.11+**
- **PlatformIO Core**: Highly recommended for multi-environment dependency management.
- **ESP32 USB Drivers**: CP210x or CH340 depending on your exact board variant.

### Software Toolchain
- **CMake 3.16+**
- **Ninja** Build System (Optional but recommended)
- **Qt 6.6.2+** Framework
  - Required Modules: `qtmultimedia`, `qtconnectivity`, `qtshadertools`
- OS-specific toolchains:
  - Linux: GCC / Clang, `libgl1-mesa-dev`, `libasound2-dev`, `libpulse-dev`
  - Windows: MSVC 2019+ or MinGW
  - Android: Android SDK (Platform 34) & NDK (25.1.8937393), Java JDK 17

---

## 2. Flashing the ESP32 Firmware

The embedded firmware uses compile-time polymorphism to select hardware configurations.

1. Open a terminal and navigate to the controller firmware directory:
   ```bash
   cd firmware/main_controller
   ```
2. Build and upload a specific hardware profile. For example, to upload the profile utilizing the MPU6050 and QMC5883L:
   ```bash
   pio run -e rover_v2_mpu6050_qmc5883l_tb6612 -t upload
   ```
3. Monitor the serial output to verify successful mDNS broadcast and sensor initialization:
   ```bash
   pio device monitor -b 115200
   ```

### Calibration
Before first operation, you must compile and run the calibration environment to zero out gyroscopes and measure magnetometer offsets:
```bash
pio run -e firmware_sensor_calibration -t upload
```
Follow the serial monitor prompts to rotate the vehicle and store values in the ESP32 NVS memory.

---

## 3. Building the LibreESPBot

The LibreESPBot utilizes a standard CMake build flow. 

### Desktop (Linux / Windows)
1. Navigate to the controller software directory:
   ```bash
   cd software/controller_app
   ```
2. Configure the project:
   ```bash
   cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
   ```
   *(Ensure `CMAKE_PREFIX_PATH` points to your Qt 6 installation directory if it is not in your system path).*
3. Build the binary:
   ```bash
   cmake --build build
   ```
4. Run the executable located in `build/RoverGroundStation`.

### Android (APK)
Building for Android requires cross-compilation against the Android NDK and QT Android libraries. It is heavily recommended to rely on our provided GitHub Actions `.github/workflows/release.yml` for automated APK generation unless you have a fully configured local Qt for Android environment.

---

## 4. Operational Workflow

1. Power on the ESP32 Rover Platform.
2. Ensure both the Rover and your Controller device (Phone/PC) are on the same Wi-Fi subnet.
3. Launch the Qt 6 `RoverGroundStation` application.
4. The `DiscoveryWorker` will automatically detect the rover via mDNS (`_roverctrl._udp.local`), establish the UDP data streams, and populate the telemetry HUD.
5. Use the Virtual Joysticks or a connected Gamepad to drive!
