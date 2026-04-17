#!/usr/bin/env python
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

ROS service which when queried with a key, 
    returns its position on the keyboard.
Used for auto typing task at URC
In Auto Mode it looks for the keyboard before 
    publishing its transform.
In Manual Mode it constantly publishes the defined
    manual keyboard transform.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: keyboard_mapper
SERVICES: get_key_position
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	arm
AUTHOR(S):  Anthony Lew
CREATION:	6/04/2024
EDITED:     6/04/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Find an IRL alignment
 - Calibrate periscope camera
 - Test with moveable arm (Arm is joints are currently locked to facilitate testing)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from geometry_msgs.msg import TransformStamped
from arm_interfaces.srv import KeyPosition
from arm_interfaces.msg import KeyboardPoints
from sensor_msgs.msg import Image
from aruco_opencv_msgs.msg import ArucoDetection

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data

from tf2_ros import Buffer, TransformListener, TransformBroadcaster
import tf2_geometry_msgs

import math
import numpy as np
from scipy.spatial.transform import Rotation as R
import cv2

# Expected keyboard size:
KEYBOARD = (123, 354, 37) # (L, W, H) in mm 
# where L is length (column of keys direction [qaz]), W is width (row of keys direction [qwertyuiop]), H is height (From base to key)
# Hard coded key coordinates in mm (relative to center of keyboard; looking down at keyboard with cable facing up) 
# left = -x, right = +x, up = -y, down = +y 
# "key": (x, y) 
KEY_MAP = { 
    "esc": (-164, -50), "f1": (-126, -50), "f2": (-107, -50), "f3": (-88, -50), "f4": (-69, -50), "f5": (-40, -50), "f6": (-21, -50), "f7": (-2, -50), "f8": (17, -50), "f9": (46, -50), "f10": (65, -50), "f11": (84, -50), "f12": (103, -50), "prtsc": (126, -50), "scrlk": (145, -50), "pause": (164, -50),
    "`": (-164, -28), "1": (-145, -28), "2": (-126, -28), "3": (-107, -28), "4": (-88, -28), "5": (-69, -28), "6": (-50, -28), "7": (-31, -28), "8": (-12, -28), "9": (7, -28), "0": (26, -28), "-": (45, -28), "=": (64, -28), "backspace": (93, -28), "ins": (126, -28), "home": (145, -28), "pgup": (164, -28),
    "tab": (-159, -9), "q": (-135, -9), "w": (-116, -9), "e": (-97, -9), "r": (-78, -9), "t": (-59, -9), "y": (-40, -9), "u": (-21, -9), "i": (-2, -9), "o": (17, -9), "p": (36, -9), "[": (55, -9), "]": (74, -9), "\\": (97, -9), "del": (126, -9), "end": (145, -9), "pgdn": (164, -9),
    "capslk": (-157, -10), "a": (-130, -10), "s": (-111, -10), "d": (-92, -10), "f": (-73, -10), "g": (-54, -10), "h": (-35, -10), "j": (-16, -10), "k": (3, -10), "l": (22, -10), ";": (41, -10), "'": (60, -10), "enter": (91, -10),
    "lshift": (-152, 29), "z": (-121, 29), "x": (-102, 29), "c": (-83, 29), "v": (-64, 29), "b": (-45, 29), "n": (-26, 29), "m": (-7, 29), ",": (12, 29), ".": (31, 29), "/": (50, 29), "rshift": (86, 29), "uarrow": (145, 29),
    "lctrl": (-161, 48), "win": (-138, 48), "lalt": (-114, 48), "space": (-44, 48), "ralt": (29, 48), "fn": (53, 48), "menu": (76, 48), "rctrl": (101, 48), "larrow": (126, 48), "darrow": (145, 48), "rarrow": (164, 48) 
}

SINGLE_KEY = (13, 15) # unused but is the the size of an individual key

DEFAULT_POSITION = [0.0, 0.0, 0.0]
DEFAULT_QUATERNION = [0.0, 0.0, 0.0, 1.0]

"""
HOW TO TEST:
nix-shell -p 'with import /home/nova/nova/nixfiles { }; pkgs.ros.nova-workspace.override {
	novaPackages = {
		inherit (pkgs.ros)
		nova-arm
		nova-arm-interfaces;
	};
}'
ros2 run arm keyboard_localiser.py --ros-args --params-file /home/nova/nova/src/ros/rover/nova_bringup/params/arm.yaml

ros2 service call /pub_key_position arm_interfaces/srv/KeyPosition '{key: "a"}'

In separate terminal:
- Run GUI
- Navigate to urc/auto-typing
"""

KEY_SERVICE_NAME = '/arm/keyboard/pub_key_position'
ARUCO_TOPIC = '/arm/aruco/detections'
POINT_TOPIC = '/arm/keyboard/points'

class KeyboardLocaliser(Node):
    def __init__(self):
        super().__init__('keyboard_localiser')

        # Choose whether to use auto transform or manual align transform
        self.node_is_auto = self.declare_parameter('using_auto', True).get_parameter_value().bool_value
        # Publisher for keyboard points to GUI
        self.point_pub = self.create_publisher(KeyboardPoints, POINT_TOPIC, 10)

        # key position initalisation
        self.keyboard_frame = self.declare_parameter('keyboard_frame', 'keyboard_frame').get_parameter_value().string_value
        self.base_frame = self.declare_parameter('base_frame', 'base_link').get_parameter_value().string_value
        self.key_srv = self.create_service(KeyPosition, KEY_SERVICE_NAME, self.publish_key_position_callback)
        self.key_map = KEY_MAP
        self.key_offset = self.declare_parameter('key_offset', 10.0).get_parameter_value().double_value # key offset in cm

        # manual keyboard alignment initalisation
        self.aligned_keyboard_position = self.declare_parameter('aligned_keyboard_position', DEFAULT_POSITION).get_parameter_value().double_array_value
        self.aligned_keyboard_quaternion = self.declare_parameter('aligned_keyboard_quaternion', DEFAULT_QUATERNION).get_parameter_value().double_array_value
        self.key_quaternion = self.declare_parameter('key_quaternion', DEFAULT_QUATERNION).get_parameter_value().double_array_value

        # calibrated camera intrinsics
        hfov = self.declare_parameter('hfov', 61.3727248).get_parameter_value().double_value
        width = self.declare_parameter('image_width', 1280).get_parameter_value().integer_value
        height = self.declare_parameter('image_height', 720).get_parameter_value().integer_value
        focal_length = width / 2 / math.tan(math.radians(hfov)/2) # for defaults = 1078.467509; 
        image_center = (width//2, height//2)
        self.camera_matrix = np.array([
            [focal_length, 0, image_center[0]],
            [0, focal_length, image_center[1]],
            [0, 0, 1]
        ], dtype=np.float32)
        dist_arr = self.declare_parameter('distortion_matrix', [0.0,0.0,0.0,0.0,0.0]).get_parameter_value().double_array_value
        self.dist_coeffs = np.array(dist_arr)
        self.camera_resolution = width, height

        # keyboard pose analysis initalisation
        self.camera_frame = self.declare_parameter('camera_frame', 'image_frame').get_parameter_value().string_value

        self.aruco_ids = list(self.declare_parameter('aruco_ids', [1, 4, 3, 2]).get_parameter_value().integer_array_value)
        self.aruco_topic = self.declare_parameter('aruco_topic', ARUCO_TOPIC).get_parameter_value().string_value
        self.aruco_sub = self.create_subscription(ArucoDetection, self.aruco_topic, self.aruco_callback, qos_profile=qos_profile_sensor_data)

        self.keyboard_points = np.array([   # Corner points of the keyboard relative to the keyboard frame in mm (center of keyboard)
            [-KEYBOARD[1]/2, -KEYBOARD[0]/2, 0],    # top-left
            [KEYBOARD[1]/2,  -KEYBOARD[0]/2, 0],    # top-right
            [KEYBOARD[1]/2,  KEYBOARD[0]/2, 0],     # bottom-right
            [-KEYBOARD[1]/2, KEYBOARD[0]/2, 0]      # bottom-left
        ], dtype=np.float32)
        self.keyboard_points_m = self.keyboard_points / 1000.0

        # OpenCV filter params
        self.dark_threshold = self.declare_parameter('dark_threshold', 120).get_parameter_value().integer_value
        self.kernel_size = self.declare_parameter('kernel_size', 100).get_parameter_value().integer_value

        # tf2 initalisation
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.transform_broadcaster = TransformBroadcaster(self)
        timer_period = self.declare_parameter('tf_publish_rate', 1.0).get_parameter_value().double_value
        self.create_timer(timer_period, self.publish_aligned_tf)

        self.get_logger().info(f"Running this node in {"auto" if self.node_is_auto else "manual"} mode with service: {KEY_SERVICE_NAME}. Using keyboard: {self.keyboard_frame} for transforms and base link: {self.base_frame}")

    def publish_key_position_callback(self, request, response):
        """ Publishes transform of the key requested from the service 
            under the frame name {key}_{keyboard_frame} and returns service with success bool
            Adds offset to key transform so that EE doesn't crash into keys (Assumes -Z is away)
        """
        key_symbol = request.key.lower()
        if key_symbol not in self.key_map:
            self.get_logger().warn(f"Key {request.key} not found in map.")
            response.success = False
            return response

        x, y = self.key_map[key_symbol]
        tfs = TransformStamped()
        tfs.header.frame_id = self.keyboard_frame
        tfs.child_frame_id = key_symbol + '_' + self.keyboard_frame
        tfs.header.stamp = request.stamp
        tfs.transform.translation.x = x * 0.001 # convert mm to meters
        tfs.transform.translation.y = y * 0.001
        tfs.transform.translation.z = -1 * self.key_offset / 100
        tfs.transform.rotation.x, tfs.transform.rotation.y, tfs.transform.rotation.z, tfs.transform.rotation.w = self.key_quaternion

        self.get_logger().info(f"Publishing transform for {request.key}")
        self.transform_broadcaster.sendTransform(tfs)
        response.success = True
        return response

    def publish_aligned_tf(self) -> None:
        '''Publish the transform of the keyboard through the alignment method'''
        if self.node_is_auto:
            return
        
        tfs = TransformStamped()
        tfs.header.stamp = self.get_clock().now().to_msg()
        tfs.header.frame_id = self.base_frame
        tfs.child_frame_id = self.keyboard_frame
        tfs.transform.translation.x, tfs.transform.translation.y, tfs.transform.translation.z, = self.aligned_keyboard_position
        tfs.transform.rotation.x, tfs.transform.rotation.y, tfs.transform.rotation.z, tfs.transform.rotation.w = self.aligned_keyboard_quaternion

        self.transform_broadcaster.sendTransform(tfs)

    def publish_analysis_tf(self) -> None:
        '''Publish the transform of the keyboard through the solvePnP method'''
        if not self.node_is_auto:
            return
        transform = self.estimate_pose()
        if transform is None:
            return None
        self.transform_broadcaster.sendTransform(transform)

    def aruco_callback(self, detection: ArucoDetection) -> None:
        """Store latest ArUco detections"""
        self.aruco_detection = detection
        self.publish_analysis_tf()
    
    def get_aruco_corners(self) -> None | np.ndarray:
        """
        Return marker centre positions as [x,y,z] in order
        """        
        marker_lookup = {marker.marker_id: marker for marker in self.aruco_detection.markers}

        ordered_points = []
        for marker_id in self.aruco_ids:
            marker = marker_lookup[marker_id]
            ordered_points.append(
                [
                    marker.pose.position.x,
                    marker.pose.position.y,
                    marker.pose.position.z
                ]
            )

        return np.array(ordered_points, dtype=np.float32)
        
    def pub_keyboard_corners(self, points) -> None:
        """ Get the sorted corners of the keyboard from the image msg """

        detected_points_m = self.get_aruco_corners()

        # Publish points
        if points is not None:
            kb_msg = KeyboardPoints()
            kb_msg.points = [int(i) for point in points for i in point]
            kb_msg.width, kb_msg.height = self.camera_resolution
            self.point_pub.publish(kb_msg)
        
    def estimate_pose(self) -> None | TransformStamped:
        """Estimate pose of keyboard and return its transform"""
        self.pub_keyboard_corners()
        ## run solvePnP
        # image_points = self.get_corners()
        # if image_points is None:
        #     return None

        # success, rvec, tvec = cv2.solvePnP(self.keyboard_points, image_points, self.camera_matrix, self.dist_coeffs)
        
        # # Convert to rotation matrix then to quaternion 
        # rotation_matrix, _ = cv2.Rodrigues(rvec)
        # rot = R.from_matrix(rotation_matrix)
        # quat = rot.as_quat()  # [x, y, z, w]
        # ## convert quaternion and transform vector to transform message
        # t = TransformStamped()
        # t.header.stamp = self.get_clock().now().to_msg()
        # t.header.frame_id = self.camera_frame
        # t.child_frame_id = self.keyboard_frame

        # t.transform.translation.x = tvec[0][0] / 1000.0  # mm → meters
        # t.transform.translation.y = tvec[1][0] / 1000.0
        # t.transform.translation.z = tvec[2][0] / 1000.0

        # t.transform.rotation.x = quat[0]
        # t.transform.rotation.y = quat[1]
        # t.transform.rotation.z = quat[2]
        # t.transform.rotation.w = quat[3]

        # return t


def main():
    rclpy.init()
    node = KeyboardLocaliser()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()