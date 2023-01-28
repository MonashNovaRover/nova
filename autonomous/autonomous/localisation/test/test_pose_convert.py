import unittest
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu
from geometry_msgs.msg import PoseWithCovariance
from nav_msgs.msg import Odometry
import numpy as np
import time
from time import perf_counter
from localisation.gps_converter import GpsConverter
from core.msg import WheelData
from rclpy.qos import qos_profile_sensor_data as qos

class FakeImuPub(Node):
    def __init__(self):
        super().__init__("imu")
        self.pub = self.create_publisher(Imu, "/imu/data", 10)

    def publish_straight(self):
        msg = Imu()
        self.pub.publish(msg)


class FakeGpsPub(Node):
    def __init__(self):
        super().__init__("gps")
        self.gps_converter = GpsConverter()
        self.pub = self.create_publisher(PoseWithCovariance, "/gps_rover/pose_cov", qos)
        self.pub_true = self.create_publisher(Odometry, "/test/odom", 10)
        self.coords = (-37., 144.)
        self.gps_converter.get_local_coord(*self.coords)
        self.pos = np.array((0., 0.))
        self.cov = np.array([[0.3, 0.], [0., 0.3]])

        self.counter = 0

        self.previous_pub = perf_counter()
        self.publish(*self.coords)

    def publish_straight(self, speed):
        self.counter += 1
        t = perf_counter()
        dt = t - self.previous_pub
        self.previous_pub = t

        if self.counter > 100:
            self.pos += np.array((speed * dt, 0))
        approx_pos = np.random.multivariate_normal(self.pos, self.cov)

        approx_gps = self.gps_converter.get_global_coord(*approx_pos)
        self.publish_true_odom(*approx_pos)
        self.publish(*approx_gps)

    def publish_true_odom(self, x, y):
        msg = Odometry()
        msg.header.frame_id = "map"
        msg.pose.pose.position.x = x
        msg.pose.pose.position.y = y

        self.pub_true.publish(msg)

    def publish(self, lat, lon):
        msg = PoseWithCovariance()
        msg.pose.position.x = float(lat)
        msg.pose.position.y = float(lon)

        msg.covariance[0] = self.cov[0, 0]
        msg.covariance[1] = self.cov[1, 0]
        msg.covariance[6] = self.cov[0, 1]
        msg.covariance[7] = self.cov[1, 1]

        self.pub.publish(msg)


class FakeDrivePub(Node):
    def __init__(self):
        super().__init__("drive")
        self.pub = self.create_publisher(WheelData, "/electronics/wheel_data", 10)
        self.counter = 0

    def publish_const(self, speed):
        # simulate slippage
        self.counter += 1
        speeds = [speed for _ in range(6)]
        data = WheelData()
        if self.counter > 100:
            data.velocities = speeds
        self.pub.publish(data)


def main():
    rclpy.init()
    imu = FakeImuPub()
    gps = FakeGpsPub()
    drive = FakeDrivePub()

    while True:
        imu.publish_straight()
        gps.publish_straight(0.5)
        drive.publish_const(0.5)

        time.sleep(0.2)

    rclpy.shutdown()

if __name__ == "__main__":
    main()
