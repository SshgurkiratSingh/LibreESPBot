# Rover Ground Station - User Manual

This manual provides an overview of the Qt 6 Ground Station interface, keyboard shortcuts, and control logic for operating the ESP32 Rover Platform.

## 1. Network Connection
Upon launching the application, it will attempt to automatically discover the rover via mDNS (`_roverctrl._udp.local`).
- If no rover is found, you will see a "VEHICLE OFFLINE / DISCONNECTED" overlay (as shown in the `docs/Images/DisconnectedApp.png` screenshot).
- Once connected, the UDP telemetry streaming begins at 50Hz, updating the Artificial Horizon and Hardware Inspector in real time.

## 2. Keyboard Control Scheme
The app supports robust keyboard controls, mapped ergonomically for WASD and quick toggles:

### Actuation (Driving)
- `W` / `Up Arrow`: Throttle Forward
- `S` / `Down Arrow`: Throttle Reverse
- `A` / `Left Arrow`: Steer Left
- `D` / `Right Arrow`: Steer Right
- `Spacebar`: Temporarily boost to **Sport Mode (100%)** while held down. Releasing it reverts to the previous speed mode.

### Speed Modes
- `1`: Crawl Mode (15%) - High precision, slow speed maneuvering indoors.
- `2`: Precision Mode (30%)
- `3`: Normal Mode (70%)
- `4`: Sport Mode (100%) - Unrestricted speed.

### Toggles & Utilities
- `B`: Toggle Auto Emergency Brake (AEB)
- `V`: Toggle APF Collision Avoidance
- `R`: Toggle Radar Sweep
- `O`: Toggle Obstacle Proximity Alerts
- `F`: Swap ToF Sensor Orientation (Reverse Front/Back)
- `K`: Toggle 3D Kinematics Wireframe Overlay
- `H` / `L`: Cycle Headlight Modes (Off, Low, High, Strobe)

## 3. UI Overlays and Features

### 3D Kinematics Wireframe (`[K]`)
When enabled, an orthographic 3D wireframe of the rover is rendered over the video stream. This wireframe is dynamically rotated based on the live IMU Pitch and Roll, with its Yaw aligned to the Magnetic Compass Heading relative to True North.

### Obstacle Proximity Alert (`[O]`)
A smart collision warning system leveraging the dual ToF sensors.
- **Trigger**: The alert only appears if an object breaches the **100mm threshold**. 
- **Filtering**: Out-of-range deadband errors (`8190` or `8191` mm) reported by the sensor are automatically filtered to prevent false-positives.
- **Context-Aware**: The alert only flashes when you are actively supplying throttle input (moving), to prevent annoyance when the vehicle is safely parked near a wall.
- **Distance Readout**: Displays the minimum distance in millimeters to the nearest obstacle.

### Reverse ToF Sensors (`[F]`)
Depending on how you mount your `VL53L0X` sensors on the chassis (front-facing vs rear-facing), you can dynamically swap their logical alignment in the software. This flips the Hardware Inspector telemetry labels between `(Front)` and `(Back)` and adjusts collision logic accordingly.

## 4. Virtual Joystick
For touch-screen or mouse operation, the right sidebar features a unified, multi-directional Virtual Joystick. It provides analog speed scaling based on the radius of the joystick pull, filtered through an exponential response curve (`u_exp(v) = sgn(v) * |v|^1.6`) for smoother low-speed handling.
