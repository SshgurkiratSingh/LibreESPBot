import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu, Range, LaserScan
from geometry_msgs.msg import Twist
from diagnostic_msgs.msg import DiagnosticArray, DiagnosticStatus, KeyValue
import socket
import struct
import threading
import math
import time

TELEMETRY_FMT = '<HBBBIfffffffhHHhhffHH'
COMMAND_FMT = '<HHhhBBBBBBBBBBBH'

class LibreEspBridge(Node):
    def __init__(self):
        super().__init__('libreesp_ros_bridge')
        
        self.declare_parameter('rover_ip', '192.168.4.1')
        self.declare_parameter('rover_port', 8888)
        self.declare_parameter('listen_port', 8888)
        
        self.rover_ip = self.get_parameter('rover_ip').value
        self.rover_port = self.get_parameter('rover_port').value
        self.listen_port = self.get_parameter('listen_port').value
        
        self.cmd_seq = 0
        
        # Publishers
        self.imu_pub = self.create_publisher(Imu, '/imu/data', 10)
        self.tof_left_pub = self.create_publisher(Range, '/sensors/range_left', 10)
        self.tof_right_pub = self.create_publisher(Range, '/sensors/range_right', 10)
        self.scan_pub = self.create_publisher(LaserScan, '/scan', 10)
        self.diag_pub = self.create_publisher(DiagnosticArray, '/diagnostics', 10)
        
        # Subscribers
        self.cmd_sub = self.create_subscription(Twist, '/cmd_vel', self.cmd_vel_callback, 10)
        
        # UDP Socket Setup
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('0.0.0.0', self.listen_port))
        self.sock.settimeout(0.5)
        
        self.running = True
        self.recv_thread = threading.Thread(target=self.receive_loop)
        self.recv_thread.start()
        
        self.get_logger().info(f'ROS 2 Bridge initialized. Listening on UDP {self.listen_port}, sending to {self.rover_ip}:{self.rover_port}')

    def calculate_crc16(self, data):
        crc = 0xFFFF
        for b in data:
            crc ^= b
            for _ in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ 0x8408
                else:
                    crc >>= 1
        return crc ^ 0xFFFF

    def receive_loop(self):
        while self.running:
            try:
                data, addr = self.sock.recvfrom(1024)
                
                # Check preamble and length
                if len(data) == struct.calcsize(TELEMETRY_FMT):
                    unpacked = struct.unpack(TELEMETRY_FMT, data)
                    if unpacked[0] == 0xAA55:
                        self.process_telemetry(unpacked)
            except socket.timeout:
                pass
            except Exception as e:
                self.get_logger().error(f"Error in receive loop: {e}")

    def process_telemetry(self, t):
        now = self.get_clock().now().to_msg()
        
        # Parse elements
        (preamble, hw_rev, imu_type, mag_type, timestamp_ms,
         pitch, roll, yaw, heading, acc_x, acc_y, acc_z,
         servo_angle, tof1_dist, tof2_dist,
         motor_left, motor_right, battery, imu_temp,
         status_flags, crc) = t
        
        # IMU Publish
        imu_msg = Imu()
        imu_msg.header.stamp = now
        imu_msg.header.frame_id = "base_link"
        imu_msg.linear_acceleration.x = float(acc_x)
        imu_msg.linear_acceleration.y = float(acc_y)
        imu_msg.linear_acceleration.z = float(acc_z)
        
        # Simple euler to quaternion for orientation
        cy = math.cos(math.radians(yaw) * 0.5)
        sy = math.sin(math.radians(yaw) * 0.5)
        cp = math.cos(math.radians(pitch) * 0.5)
        sp = math.sin(math.radians(pitch) * 0.5)
        cr = math.cos(math.radians(roll) * 0.5)
        sr = math.sin(math.radians(roll) * 0.5)
        
        imu_msg.orientation.w = cr * cp * cy + sr * sp * sy
        imu_msg.orientation.x = sr * cp * cy - cr * sp * sy
        imu_msg.orientation.y = cr * sp * cy + sr * cp * sy
        imu_msg.orientation.z = cr * cp * sy - sr * sp * cy
        
        self.imu_pub.publish(imu_msg)
        
        # Range Left Publish
        r_left = Range()
        r_left.header.stamp = now
        r_left.header.frame_id = "tof_left_link"
        r_left.radiation_type = Range.INFRARED
        r_left.field_of_view = 0.436 # approx 25 deg
        r_left.min_range = 0.05
        r_left.max_range = 2.0
        r_left.range = float(tof1_dist) / 1000.0
        self.tof_left_pub.publish(r_left)
        
        # Range Right Publish
        r_right = Range()
        r_right.header.stamp = now
        r_right.header.frame_id = "tof_right_link"
        r_right.radiation_type = Range.INFRARED
        r_right.field_of_view = 0.436
        r_right.min_range = 0.05
        r_right.max_range = 2.0
        r_right.range = float(tof2_dist) / 1000.0
        self.tof_right_pub.publish(r_right)
        
        # Diagnostics
        diag_arr = DiagnosticArray()
        diag_arr.header.stamp = now
        
        diag = DiagnosticStatus()
        diag.name = "Rover Hardware Status"
        diag.level = DiagnosticStatus.OK
        if status_flags & 0x08: # Failsafe
            diag.level = DiagnosticStatus.ERROR
            diag.message = "Failsafe Triggered!"
        elif status_flags & 0x01: # Obstacle
            diag.level = DiagnosticStatus.WARN
            diag.message = "Obstacle Detected"
        else:
            diag.message = "OK"
            
        diag.values.append(KeyValue(key="Battery Voltage", value=f"{battery:.2f}V"))
        diag.values.append(KeyValue(key="IMU Temperature", value=f"{imu_temp:.1f}C"))
        
        diag_arr.status.append(diag)
        self.diag_pub.publish(diag_arr)

    def cmd_vel_callback(self, msg):
        # Convert twist to throttle/steering PWM
        # Max Twist X usually 1.0 m/s
        # Max PWM is 1023
        throttle = int(max(min(msg.linear.x * 1023.0, 1023), -1023))
        # Twist Z for steering (yaw rate)
        steering = int(max(min(msg.angular.z * 1023.0, 1023), -1023))
        
        self.cmd_seq = (self.cmd_seq + 1) & 0xFFFF
        
        # Prepare struct
        data = struct.pack(COMMAND_FMT[:-1], # Pack without CRC first
                           0x55AA, # preamble
                           self.cmd_seq,
                           throttle,
                           steering,
                           0, # enableAutoBrake
                           0, # enableApfAvoidance
                           0, # enableRadarSweep
                           0, # radarSweepSpeed
                           2, # speedModeLimit (Sport)
                           0, # headlightMode
                           0, # customLedR
                           0, # customLedG
                           0, # customLedB
                           0, # customLedPattern
                           0  # enableNoLagMode
                           )
        
        crc = self.calculate_crc16(data)
        data += struct.pack('<H', crc)
        
        self.sock.sendto(data, (self.rover_ip, self.rover_port))

    def destroy_node(self):
        self.running = False
        self.recv_thread.join()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = LibreEspBridge()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
