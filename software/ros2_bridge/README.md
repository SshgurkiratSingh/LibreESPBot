# LibreESPBot ROS 2 Bridge

This package provides a ROS 2 bridge for the LibreESPBot. It allows you to interface with the rover using standard ROS 2 tools and message types (`geometry_msgs/Twist`, `sensor_msgs/Imu`, etc.) without modifying the ESP firmware or losing compatibility with the QML Dashboard.

## Features

- **Subscribes to `/cmd_vel`**: Converts Twist messages into `VehicleCommandPacket`s and sends them to the rover.
- **Publishes `/imu/data`**: Converts the rover's IMU data into a standard ROS 2 Imu message.
- **Publishes `/sensors/range_left` & `/sensors/range_right`**: Exposes the dual ToF sensors.
- **Publishes `/diagnostics`**: Reports battery voltage, IMU temperature, and failsafe/obstacle warnings.

## Prerequisites

- ROS 2 (Humble or newer recommended)
- Python 3 `struct` and `socket` modules (built-in)

## Building the Package

You can build this package in your standard ROS 2 workspace:

```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
ln -s /path/to/ESP266-Car/software/ros2_bridge/ libreesp_ros_bridge
cd ~/ros2_ws
colcon build --packages-select libreesp_ros_bridge
source install/setup.bash
```

## Running the Bridge

Make sure you are connected to the same network as the rover.

```bash
ros2 run libreesp_ros_bridge bridge_node --ros-args -p rover_ip:="192.168.4.1" -p rover_port:=8888
```

You can then test sending commands using standard ROS 2 CLI tools:

```bash
ros2 topic pub /cmd_vel geometry_msgs/Twist "{linear: {x: 0.5, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 0.5}}" -1
```
