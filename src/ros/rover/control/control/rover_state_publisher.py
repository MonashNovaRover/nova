#!/usr/bin/env python3

from math import sin, cos, pi
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile
from geometry_msgs.msg import Quaternion
from sensor_msgs.msg import JointState
from core.msg import Telemetry, SingleTelemetry
from tf2_ros import TransformBroadcaster, TransformStamped

FRONT_LEFT_DIR = 1
BACK_LEFT_DIR = 1
FRONT_RIGHT_DIR = 1
BACK_RIGHT_DIR = 1

FRONT_LEFT_PIVOT = -1
BACK_LEFT_PIVOT = -1
FRONT_RIGHT_PIVOT = -1
BACK_RIGHT_PIVOT = -1

# For getting true resolver zeros of front facing wheel
FRONT_LEFT_PIVOT_ZERO = 0.
FRONT_RIGHT_PIVOT_ZERO = 0.
BACK_LEFT_PIVOT_ZERO = 0.
BACK_RIGHT_PIVOT_ZERO = 0.

class RoverStatePublisher(Node):
    def __init__(self):
        super().__init__('rover_state_publisher')

        self.param_pub_frequency = self.declare_parameter("joint_pub_rate_hz", 30).value

        # For subscribing to true telemetry
        qos_profile = QoSProfile(depth=10)
        self.joint_pub = self.create_publisher(JointState, 'joint_states', qos_profile)

        # Broadcasting joint transforms
        self.broadcaster = TransformBroadcaster(self, qos=qos_profile)

        # Telemetry callback to set joint state
        self.telemetry_sub = self.create_subscription(Telemetry, '/control/telemetry', self.telemetry_callback, qos_profile)

        # Get initial telemetry so we can visualise even when not receiving messages
        self.telemetry = None
        self.joint_state = None
        self.initialise_telemetry()
        self.calculate_joint_state()

        self.timer = self.create_timer(1/self.param_pub_frequency, self.cb_pub_joint_state)

        self.nodeName = self.get_name()
        self.get_logger().info("{0} started".format(self.nodeName))

    def initialise_telemetry(self):
        self.telemetry = Telemetry()
        self.telemetry.wheels = [SingleTelemetry() for _ in range(4)]
        self.telemetry.pivots = [SingleTelemetry() for _ in range(4)]

    def telemetry_callback(self, msg):
        self.telemetry = msg
        self.calculate_joint_state()

    def calculate_joint_state(self):
        self.joint_state = JointState()
        now = self.get_clock().now()

        self.joint_state.header.stamp = now.to_msg()

        self.joint_state.name = ['front_left_wheel_to_pivot', 'front_right_wheel_to_pivot',
                            'back_left_wheel_to_pivot', 'back_right_wheel_to_pivot',
                            'front_left_pivot_to_leg', 'front_right_pivot_to_leg',
                            'back_left_pivot_to_leg', 'back_right_pivot_to_leg']

        self.joint_state.position = [FRONT_LEFT_DIR*self.telemetry.wheels[0].resolver_position,
                                FRONT_RIGHT_DIR*self.telemetry.wheels[1].resolver_position,
                                BACK_LEFT_DIR*self.telemetry.wheels[2].resolver_position,
                                BACK_RIGHT_DIR*self.telemetry.wheels[3].resolver_position,
                                FRONT_LEFT_PIVOT*self.telemetry.pivots[0].resolver_position + FRONT_LEFT_PIVOT_ZERO,
                                FRONT_RIGHT_PIVOT*self.telemetry.pivots[1].resolver_position + FRONT_RIGHT_PIVOT_ZERO,
                                BACK_LEFT_PIVOT*self.telemetry.pivots[2].resolver_position + BACK_LEFT_PIVOT_ZERO,
                                BACK_RIGHT_PIVOT*self.telemetry.pivots[3].resolver_position + BACK_RIGHT_PIVOT_ZERO]

    def cb_pub_joint_state(self):
        self.joint_state.header.stamp=self.get_clock().now().to_msg()
        self.joint_pub.publish(self.joint_state)


def euler_to_quaternion(roll, pitch, yaw):
    qx = sin(roll/2) * cos(pitch/2) * cos(yaw/2) - cos(roll/2) * sin(pitch/2) * sin(yaw/2)
    qy = cos(roll/2) * sin(pitch/2) * cos(yaw/2) + sin(roll/2) * cos(pitch/2) * sin(yaw/2)
    qz = cos(roll/2) * cos(pitch/2) * sin(yaw/2) - sin(roll/2) * sin(pitch/2) * cos(yaw/2)
    qw = cos(roll/2) * cos(pitch/2) * cos(yaw/2) + sin(roll/2) * sin(pitch/2) * sin(yaw/2)
    return Quaternion(x=qx, y=qy, z=qz, w=qw)

def main(args=None):
    rclpy.init(args = args)
    node = RoverStatePublisher()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
