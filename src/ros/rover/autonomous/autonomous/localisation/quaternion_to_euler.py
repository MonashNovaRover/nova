#!/usr/bin/python3
import rclpy
import math
from geometry_msgs.msg import Quaternion

def quaternion_callback(msg):
    # Convert quaternion to Euler angles
    roll, pitch, yaw = quaternion_to_euler(msg)

    # Print the Euler angles
    node.get_logger().info(f'Roll: {roll}, Pitch: {pitch}, Yaw: {yaw}')

def quaternion_to_euler(q):
    q = q.orientation
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
    node = rclpy.create_node('quaternion_to_euler_node')

    # Subscribe to the Quaternion topic
    subscription = node.create_subscription(
        Quaternion,
        '/oak/imu/data',
        quaternion_callback,
        10
    )

    rclpy.spin(node)

    # Clean up
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()

