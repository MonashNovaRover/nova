#!usr/bin/env python3

import rclpy
from rcply.node import Node
from core.msg import DriveCmd, RoverPose, Waypoint
import random
import numpy as np

class TestController(Node):
    def __init__(self):
        super().__init__("autonomous_controller_test_node")

        self.pose_publisher = self.create_publisher(RoverPose, "autonomous/pose", 10)
        self.waypt_publisher = self.create_publisher(Waypoint, "autonomous/goals", 10)
        self.timer = self.create_timer(2, self.callback_func)

    def callback_func(self):
        self.publish_waypoint()
        self.publish_poses()

    def publish_waypoint(self):
        x = random.randint(-20, 20)
        y = random.randint(-20, 20)

        msg = Waypoint()
        msg.data.x = x
        msg.data.y = y

        self.waypt_publisher.publish(msg)

        print("published waypoint: x = %.2f, y = %.2f" % x, y)

    def publish_pose(self):
        x = random.randint(-20, 20)
        y = random.randint(-20, 20)
        yaw = random.randrange(-np.pi, np.pi)
        vel = 0
        ang_vel = 0

        msg = RoverPose()

        msg.data.x = x
        msg.data.y = y
        msg.data.yaw = yaw
        msg.data.velocity = vel
        msg.data.angular_velocity = ang_vel

        self.pose_publisher.publish(msg)

        print("published pose: x = %.2f, y = %.2f, yaw = %.2f, vel = %.2f, omega = %.2f" % x, y, yaw, vel, ang_vel)

def main(args = None):
    rclpy.init(args = args)
    test_controller = TestController()

    rclpy.spin(test_controller)

    test_controller.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()