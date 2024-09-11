#!/usr/bin/python3
import rclpy
from rclpy.node import Node
import math
#from geometry_msgs.msg import Quaternion
from sensor_msgs.msg import Imu

class QuaternionToEuler(Node):
    def __init__(self):
        super().__init__('quaternion_to_euler')

        self.declare_parameter('imu_topic', '/oak/imu/transformed')

        self.subscription = self.create_subscription(
            Imu,
            self.get_parameter('imu_topic').get_parameter_value().string_value,
            self.quaternion_callback,
            10
        )

    def quaternion_callback(self, msg):
        q = msg.orientation
        # Convert quaternion to Euler angles
        roll, pitch, yaw = self.quaternion_to_euler(q)

        # Print the Euler angles
        self.get_logger().info(f'Roll: {roll}, Pitch: {pitch}, Yaw: {yaw}')

    def quaternion_to_euler(self, q):
        roll = 0.0
        pitch = 0.0
        yaw = 0.0

        # Convert quaternion to Euler angles
        t0 = +2.0 * (q.w * q.x + q.y * q.z)
        t1 = +1.0 - 2.0 * (q.x**2 + q.y**2)
        roll = math.atan2(t0, t1)

        t2 = +2.0 * (q.w * q.y - q.z * q.x)
        t2 = +1.0 if t2 > +1.0 else t2
        t2 = -1.0 if t2 < -1.0 else t2
        pitch = math.asin(t2)

        t3 = +2.0 * (q.w * q.z + q.x * q.y)
        t4 = +1.0 - 2.0 * (q.y**2 + q.z**2)
        yaw = math.atan2(t3, t4)

        return roll, pitch, yaw

def main(args=None):
    rclpy.init(args=args)
    node = QuaternionToEuler()

    rclpy.spin(node)

    # Clean up
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()

