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

from geometry_msgs.msg import PointStamped, TransformStamped
from arm_interfaces.srv import KeyPosition, StringTrigger
from arm_interfaces.msg import KeyboardPoints
from sensor_msgs.msg import Image

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data

from tf2_ros import Buffer, TransformListener, TransformBroadcaster
import tf2_geometry_msgs

import math
import numpy as np
from scipy.spatial.transform import Rotation as R
import cv2
from cv_bridge import CvBridge

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
IMAGE_TOPIC = '/arm/periscope'
DEBUG_TOPIC = '/arm/keyboard/image'
POINT_TOPIC = '/arm/keyboard/points'


class KeyboardLocaliser(Node):
    def __init__(self):
        super().__init__('keyboard_localiser')

        # Choose whether to use auto transform or manual align transform
        self.node_is_auto = self.declare_parameter('using_auto', True).get_parameter_value().bool_value
        # Do we publish the debug image or keyboard points to GUI
        self.do_debug = self.declare_parameter('debug_image', True).get_parameter_value().bool_value
        self.point_pub = self.create_publisher(KeyboardPoints, POINT_TOPIC, 10)
        self.debug_pub = self.create_publisher(Image, DEBUG_TOPIC, 10) if self.do_debug else None

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
        self.camera_frame = self.declare_parameter('camera_frame', 'arm_end_periscope_optical').get_parameter_value().string_value
        self.view = None
        self.view_sub = self.create_subscription(Image, IMAGE_TOPIC, self.view_callback, qos_profile=qos_profile_sensor_data)
        self.keyboard_points = np.array([   # Corner points of the keyboard relative to the keyboard frame in mm (center of keyboard)
            [-KEYBOARD[1]/2, -KEYBOARD[0]/2, 0],    # top-left
            [KEYBOARD[1]/2,  -KEYBOARD[0]/2, 0],    # top-right
            [KEYBOARD[1]/2,  KEYBOARD[0]/2, 0],     # bottom-right
            [-KEYBOARD[1]/2, KEYBOARD[0]/2, 0]      # bottom-left
        ], dtype=np.float32)

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


    def get_transform_to_base_link(self, key_pos):
        """ 
        Auto transform functionality to automatically transform key position 
        from relative to keyboard frame to relative to base link 
        so that position can be fed directly to path planner/ik
        """
        try:
            base_to_keyboard = self.tf_buffer.lookup_transform(self.base_frame, self.keyboard_frame, key_pos.header.stamp)
            transformed_point = tf2_geometry_msgs.do_transform_point(key_pos, base_to_keyboard)
        except:
            return None

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
        if self.view is None or not self.node_is_auto:
            return
        transform = self.estimate_pose()
        if transform is None:
            return None
        self.transform_broadcaster.sendTransform(transform)

    def view_callback(self, view) -> None:
        """ Callback when Image is recieved (Stores msg for retrieval) """
        self.view = view
        self.publish_analysis_tf()
    
    def msg_to_mat(self, logger, img, encoding) -> np.ndarray:
        """Converts Image msg to cv2 frame"""
        mat = None
        try:
            mat = CvBridge().imgmsg_to_cv2(img, encoding)
        except Exception as e:
            logger.error(str(e))
        return mat

    def get_corners(self) -> None | np.ndarray:
        """ Get the sorted corners of the keyboard from the image msg """
        image = self.msg_to_mat(self.get_logger(), self.view, 'bgr8')

        # Get filtered mask
        hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
        h, s, v = cv2.split(hsv)

        # filter for dark colours (black keyboard)
        _, mask = cv2.threshold(v, self.dark_threshold, 255, cv2.THRESH_BINARY_INV)

        # close small gaps in mask
        kernel_close = cv2.getStructuringElement(cv2.MORPH_RECT, (5,5))
        mask_closed = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel_close, iterations=2)

        # filter out irregular blobs
        kern_neck = cv2.getStructuringElement(cv2.MORPH_RECT, (3, self.kernel_size))
        mask_pruned = cv2.morphologyEx(mask_closed, cv2.MORPH_OPEN, kern_neck, iterations=1)
        kern_blip = cv2.getStructuringElement(cv2.MORPH_RECT, (self.kernel_size, 3))
        mask_blip = cv2.morphologyEx(mask_pruned, cv2.MORPH_OPEN, kern_blip, iterations=1)
        
        # Get largest contour's hull
        approx = None
        contours, _ = cv2.findContours(mask_blip, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        if contours:
            hull = cv2.convexHull(max(contours, key=cv2.contourArea))
            # simplify contour polygon using algorithm (0.02 *cv2 arcLength is 2% of perimeter)
            approx = cv2.approxPolyDP(hull, 0.02 * cv2.arcLength(hull, True), True)

        # Sort the points in order: top-left, top-right, bottom-right, bottom-left
        def sort_corners(approx) -> np.ndarray:
            if approx is None or len(approx) != 4:
                return None
            # Extract and reshape points to 2D array
            pts = approx.reshape(4, 2)

            sorted_pts = np.zeros((4, 2), dtype="float32")
            s = pts.sum(axis=1)
            diff = np.diff(pts, axis=1)
            sorted_pts[0] = pts[np.argmin(s)]       # top-left
            sorted_pts[2] = pts[np.argmax(s)]       # bottom-right
            sorted_pts[1] = pts[np.argmin(diff)]    # top-right
            sorted_pts[3] = pts[np.argmax(diff)]    # bottom-left
            return sorted_pts

        image_points = sort_corners(approx)
        self.pub_debug_image(image, image_points)
        return image_points
    
    def pub_debug_image(self, img, points) -> None:
        # Publish points and draw each point
        if points is not None:
            kb_msg = KeyboardPoints()
            kb_msg.points = [int(i) for point in points for i in point]
            kb_msg.width, kb_msg.height = self.camera_resolution
            self.point_pub.publish(kb_msg)
            for point in points:
                cv2.circle(img, (int(point[0]), int(point[1])), radius=5, color=(0, 255, 0), thickness=-1)

        if not self.do_debug:
            return

        # Convert OpenCV image -> ROS Image
        output_msg = CvBridge().cv2_to_imgmsg(img, encoding="bgr8")

        # Publish the image
        self.debug_pub.publish(output_msg)
        
    def estimate_pose(self) -> None | TransformStamped:
        """Estimate pose of keyboard and return its transform"""
        ## run solvePnP
        image_points = self.get_corners()
        if image_points is None:
            return None

        success, rvec, tvec = cv2.solvePnP(self.keyboard_points, image_points, self.camera_matrix, self.dist_coeffs)
        
        # Convert to rotation matrix then to quaternion 
        rotation_matrix, _ = cv2.Rodrigues(rvec)
        rot = R.from_matrix(rotation_matrix)
        quat = rot.as_quat()  # [x, y, z, w]
        ## convert quaternion and transform vector to transform message
        t = TransformStamped()
        t.header.stamp = self.get_clock().now().to_msg()
        t.header.frame_id = self.camera_frame
        t.child_frame_id = self.keyboard_frame

        t.transform.translation.x = tvec[0][0] / 1000.0  # mm → meters
        t.transform.translation.y = tvec[1][0] / 1000.0
        t.transform.translation.z = tvec[2][0] / 1000.0

        t.transform.rotation.x = quat[0]
        t.transform.rotation.y = quat[1]
        t.transform.rotation.z = quat[2]
        t.transform.rotation.w = quat[3]

        return t


def main():
    rclpy.init()
    node = KeyboardLocaliser()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()