#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Reads magnetometer data and calculates
the true heading based on GPS coordinates and
magnetic declination.

Written for the HMC5883L magnetometer, which is
mounted upside down on the rover, so the magnetic
heading is flipped by 180 degrees.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: magnetometer_node
TOPICS:
  - subscriber: /gps_rover/fix         [NavSatFix]
  - publisher: /mag/heading            [Float64]
  - publisher: /gps_rover/imu_heading  [Imu]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Terry Tian
CREATED:	15/04/2026
EDITED:		15/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import math
from datetime import date
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float64
from sensor_msgs.msg import NavSatFix, Imu
from geometry_msgs.msg import Quaternion
from rclpy.qos import QoSPresetProfiles
from smbus2 import SMBus
from wmm import wmm_calc

def s16(msb, lsb):
    v = (msb << 8) | lsb
    return v - 65536 if v >= 32768 else v

class MagnetometerNode(Node):
    def __init__(self):
        super().__init__('magnetometer_node')
        self.declare_parameter('bus', 7)
        self.declare_parameter('addr', 0x1E)
        self.bus_num = self.get_parameter('bus').value
        self.addr = self.get_parameter('addr').value

        self.publisher = self.create_publisher(Float64, '/mag/heading', QoSPresetProfiles.SENSOR_DATA.value)
        self.imu_publisher = self.create_publisher(Imu, '/gps_rover/imu_heading', QoSPresetProfiles.SENSOR_DATA.value)
        self.subscription = self.create_subscription(NavSatFix, '/gps_rover/fix', self.gps_callback,
                                                     QoSPresetProfiles.SENSOR_DATA.value)
        self.timer = self.create_timer(0.1, self.timer_callback)  # 10 Hz

        self.bus = SMBus(self.bus_num)
        self.bus.write_byte_data(self.addr, 0x00, 0x70)
        self.bus.write_byte_data(self.addr, 0x01, 0x20)
        self.bus.write_byte_data(self.addr, 0x02, 0x00)

        self.model = wmm_calc()
        today = date.today()
        self.model.setup_time([today.year], [today.month], [today.day])

        self.lat = None
        self.lon = None
        self.alt_km = 0.0
        self.declination_deg = 0.0

    def gps_callback(self, msg):
        self.lat = msg.latitude
        self.lon = msg.longitude
        self.alt_km = msg.altitude / 1000.0  # Convert to km
        self.get_logger().info(f'Received GPS fix: lat={self.lat:.6f} '\
                               f'lon={self.lon:.6f}, alt={self.alt_km:.3f} km',
                               throttle_duration_sec=1.0)
        if self.lat is not None and self.lon is not None:
            self.model.setup_env([self.lat], [self.lon], [self.alt_km])
            self.declination_deg = float(self.model.get_Bdec()[0])

    def timer_callback(self):
        data = self.bus.read_i2c_block_data(self.addr, 0x03, 6)
        mx = s16(data[0], data[1])
        mz = s16(data[2], data[3])
        my = s16(data[4], data[5])

        # Heading is flipped because the sensor is mounted upside down
        magnetic_heading = -math.degrees(math.atan2(my, mx)) % 360.0
        true_heading = (magnetic_heading - self.declination_deg) % 360.0
        self.get_logger().info(f'Magnetic Heading: {magnetic_heading:.2f}° ' \
                                f'Declination: {self.declination_deg:.2f}° ' \
                                f'True Heading: {true_heading:.2f}°', throttle_duration_sec=1.0)

        msg = Float64()
        msg.data = true_heading
        self.publisher.publish(msg)

        # Publish magnetometer as IMU
        imu_msg = Imu()
        imu_msg.header.stamp = self.get_clock().now().to_msg()
        imu_msg.header.frame_id = 'base_link'

        true_heading_rad = math.radians(true_heading)
        half_yaw = true_heading_rad / 2.0
        imu_msg.orientation = Quaternion(
            x=0.0,
            y=0.0,
            z=math.sin(half_yaw),
            w=math.cos(half_yaw),
        )
        imu_msg.orientation_covariance = [-1.0, 0.0, 0.0,
                                           0.0,-1.0, 0.0,
                                           0.0, 0.0, 0.05]
        imu_msg.angular_velocity_covariance[0] = -1.0
        imu_msg.linear_acceleration_covariance[0] = -1.0
        self.imu_publisher.publish(imu_msg)

def main(args=None):
    rclpy.init(args=args)
    node = MagnetometerNode()
    rclpy.spin(node)
    node.bus.close()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
