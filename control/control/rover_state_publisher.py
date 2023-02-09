#!/usr/bin/env python3

from math import sin, cos, pi
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile
from geometry_msgs.msg import Quaternion
from sensor_msgs.msg import JointState
from core.msg import Telemetry
from tf2_ros import TransformBroadcaster, TransformStamped

FRONT_LEFT_DIR = 1
BACK_LEFT_DIR = 1
FRONT_RIGHT_DIR = 1
BACK_RIGHT_DIR = 1

FRONT_LEFT_PIVOT = -1
BACK_LEFT_PIVOT = -1
FRONT_RIGHT_PIVOT = -1
BACK_RIGHT_PIVOT = -1

class RoverStatePublisher(Node):
    def __init__(self):
        rclpy.init()
        super().__init__('rover_state_publisher')

        qos_profile = QoSProfile(depth=10)
        self.joint_pub = self.create_publisher(JointState, 'joint_states', qos_profile)
        self.broadcaster = TransformBroadcaster(self, qos=qos_profile)
        self.telemetry_sub = self.create_subscription(Telemetry, '/control/telemetry', self.telemetry_callback, qos_profile)
        self.nodeName = self.get_name()
        self.get_logger().info("{0} started".format(self.nodeName))


    def telemetry_callback(self, msg):

        joint_state = JointState()
        now = self.get_clock().now()

        joint_state.header.stamp = now.to_msg()

        joint_state.name = ['front_left_wheel_to_pivot', 'front_right_wheel_to_pivot',
                            'back_left_wheel_to_pivot', 'back_right_wheel_to_pivot',
                            'front_left_pivot_to_leg', 'front_right_pivot_to_leg',
                            'back_left_pivot_to_leg', 'back_right_pivot_to_leg']

        joint_state.position = [FRONT_LEFT_DIR*msg.wheels[0].resolver_position,
                                FRONT_RIGHT_DIR*msg.wheels[1].resolver_position,
                                BACK_LEFT_DIR*msg.wheels[2].resolver_position,
                                BACK_RIGHT_DIR*msg.wheels[3].resolver_position,
                                FRONT_LEFT_PIVOT*msg.pivots[0].resolver_position,
                                FRONT_RIGHT_PIVOT*msg.pivots[1].resolver_position,
                                BACK_LEFT_PIVOT*msg.pivots[2].resolver_position,
                                BACK_RIGHT_PIVOT*msg.pivots[3].resolver_position]

        self.joint_pub.publish(joint_state)

def euler_to_quaternion(roll, pitch, yaw):
    qx = sin(roll/2) * cos(pitch/2) * cos(yaw/2) - cos(roll/2) * sin(pitch/2) * sin(yaw/2)
    qy = cos(roll/2) * sin(pitch/2) * cos(yaw/2) + sin(roll/2) * cos(pitch/2) * sin(yaw/2)
    qz = cos(roll/2) * cos(pitch/2) * sin(yaw/2) - sin(roll/2) * sin(pitch/2) * cos(yaw/2)
    qw = cos(roll/2) * cos(pitch/2) * cos(yaw/2) + sin(roll/2) * sin(pitch/2) * sin(yaw/2)
    return Quaternion(x=qx, y=qy, z=qz, w=qw)

def main(args=None):
    # rclpy.init(args = args)
    node = RoverStatePublisher()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()