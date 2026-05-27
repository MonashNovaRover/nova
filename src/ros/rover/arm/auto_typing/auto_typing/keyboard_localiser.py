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
AUTHOR(S):  Anthony Lew, Binuda Kalugalage
CREATION:	6/04/2024
EDITED:     18/04/2026
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
from aruco_opencv_msgs.msg import ArucoDetection
from sensor_msgs.msg import CameraInfo, Image

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data

from tf2_ros import Buffer, TransformListener, TransformBroadcaster

import math
import numpy as np
from scipy.spatial.transform import Rotation as R
import cv2

# Minimum number of observation pairs needed for hand-eye calibration
MIN_HANDEYE_SAMPLES = 5
# Minimum angular difference (radians) between observations to count as distinct
# ~15 degrees — OpenCV needs large rotations for hand-eye to converge
MIN_ROTATION_DELTA = 0.26
# Maximum number of observation pairs to keep in the rolling buffer
MAX_HANDEYE_SAMPLES = 20

# Expected keyboard size:
KEYBOARD = (123, 354, 37) # (L, W, H) in mm 
# where L is length (column of keys direction [qaz]), W is width (row of keys direction [qwertyuiop]), H is height (From base to key)
# Hard coded key coordinates in mm (relative to center of keyboard; looking down at keyboard with cable facing up) 
# left = -x, right = +x, up = -y, down = +y 
# "key": (x, y) 
KEY_MAP = { 
    "!": (-200, 82.5), "@": (-200, -82.5), "#": (200, -82.5), "$": (200, 82.5),"&": (0, 0), "esc": (-164, -50), "f1": (-126, -50), "f2": (-107, -50), "f3": (-88, -50), "f4": (-69, -50), "f5": (-40, -50), "f6": (-21, -50), "f7": (-2, -50), "f8": (17, -50), "f9": (46, -50), "f10": (65, -50), "f11": (84, -50), "f12": (103, -50), "prtsc": (126, -50), "scrlk": (145, -50), "pause": (164, -50),
    "`": (-164, -28), "1": (-145, -28), "2": (-126, -28), "3": (-107, -28), "4": (-88, -28), "5": (-69, -28), "6": (-50, -28), "7": (-31, -28), "8": (-12, -28), "9": (7, -28), "0": (26, -28), "-": (45, -28), "=": (64, -28), "backspace": (93, -28), "ins": (126, -28), "home": (145, -28), "pgup": (164, -28),
    "tab": (-159, -9), "q": (-135, -9), "w": (-116, -9), "e": (-97, -9), "r": (-78, -9), "t": (-59, -9), "y": (-40, -9), "u": (-21, -9), "i": (-2, -9), "o": (17, -9), "p": (36, -9), "[": (55, -9), "]": (74, -9), "\\": (97, -9), "del": (126, -9), "end": (145, -9), "pgdn": (164, -9),
    "capslk": (-157, 10), "a": (-130, 10), "s": (-111, 10), "d": (-92, 10), "f": (-73, 10), "g": (-54, 10), "h": (-35, 10), "j": (-16, 10), "k": (3, 10), "l": (22, 10), ";": (41, 10), "'": (60, 10), "enter": (91, 10),
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
ARUCO_TOPIC = '/aruco_detections'
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

        # Camera intrinsics — from RealSense CameraInfo when use_depth, otherwise from manual params
        self.camera_matrix = None
        self.dist_coeffs = None
        self.camera_resolution = None
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
        dist_arr = self.declare_parameter('distortion_matrix', [0.000477749236441667163, -0.06869748182906846, -0.0030440664969761, 0.00015872921312327083, -0.35803596544161447]).get_parameter_value().double_array_value
        self.dist_coeffs = np.array(dist_arr)
        self.camera_resolution = (width, height)

        # keyboard pose analysis initalisation
        self.camera_frame = self.declare_parameter('camera_frame', 'image_frame').get_parameter_value().string_value

        self.aruco_detection = None
        self.marker_ids = list(self.declare_parameter('marker_ids', [1, 4, 3, 2]).get_parameter_value().integer_array_value)
        self.aruco_topic = self.declare_parameter('aruco_topic', ARUCO_TOPIC).get_parameter_value().string_value
        self.aruco_sub = self.create_subscription(ArucoDetection, self.aruco_topic, self.aruco_callback, qos_profile=qos_profile_sensor_data)

        marker_offset = self.declare_parameter('marker_offset', 10.0).get_parameter_value().double_value  # mm
        self.keyboard_points = np.array([
            [-(KEYBOARD[1]/2 - marker_offset), -(KEYBOARD[0]/2 - marker_offset), 0],  # top-left
            [ (KEYBOARD[1]/2 - marker_offset), -(KEYBOARD[0]/2 - marker_offset), 0],  # top-right
            [ (KEYBOARD[1]/2 - marker_offset),  (KEYBOARD[0]/2 - marker_offset), 0],  # bottom-right
            [-(KEYBOARD[1]/2 - marker_offset),  (KEYBOARD[0]/2 - marker_offset), 0],  # bottom-left
        ], dtype=np.float32)
        self.keyboard_points_m = self.keyboard_points / 1000.0

        # tf2 initalisation
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.transform_broadcaster = TransformBroadcaster(self)

        timer_period = self.declare_parameter('tf_publish_rate', 1.0).get_parameter_value().double_value
        self.create_timer(timer_period, self.publish_aligned_tf)

        # Hand-eye calibration
        self.use_handeye = self.declare_parameter('use_handeye', True).get_parameter_value().bool_value
        self.ee_frame = self.declare_parameter('ee_frame', 'image_frame').get_parameter_value().string_value
        self.handeye_marker_id = self.declare_parameter('handeye_marker_id', 1).get_parameter_value().integer_value
        # Rotation offset (degrees around Z) from marker frame to keyboard frame
        marker_yaw_offset = self.declare_parameter('handeye_marker_yaw_offset', 0.0).get_parameter_value().double_value
        self.R_marker_to_keyboard = R.from_euler('z', marker_yaw_offset, degrees=True).as_matrix()
        self.handeye_buffer = []  # list of (R_base_ee, t_base_ee, R_cam_target, t_cam_target)
        self.last_ee_rotation = None
        self.handeye_calibrated = False
        self.calibrated_quat = None  # latest calibrated rotation as [x, y, z, w]
        # Cache URDF's initial guess for image_frame (so we don't read back our own published value)
        self.R_ee2cam_urdf = None
        self.t_ee2cam_urdf = None
        if self.use_handeye:
            self.create_timer(1.0, self.cache_urdf_transform)
            # Publish calibrated transform at 10Hz to override robot_state_publisher's static TF
            self.create_timer(0.1, self.publish_calibrated_camera_tf)

        # Depth-based marker refinement
        self.use_depth = self.declare_parameter('use_depth', False).get_parameter_value().bool_value
        self.depth_image = None
        if self.use_depth:
            depth_topic = self.declare_parameter('depth_topic', '/d415/aligned_depth_to_color/image_raw').get_parameter_value().string_value
            camera_info_topic = self.declare_parameter('camera_info_topic', '/d415/color/camera_info').get_parameter_value().string_value
            self.create_subscription(Image, depth_topic, self.depth_callback, qos_profile=qos_profile_sensor_data)
            self.create_subscription(CameraInfo, camera_info_topic, self.camera_info_callback, 10)
            self.get_logger().info(f"Depth refinement enabled: {depth_topic}")

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
        tfs.child_frame_id = key_symbol + "_frame"
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
        '''Publish the transform of the keyboard through the solvePnP method,
        and collect hand-eye calibration samples.'''
        if not self.node_is_auto:
            return
        transform = self.estimate_pose()
        if transform is not None:
            self.transform_broadcaster.sendTransform(transform)
        if self.use_handeye:
            # Always use single marker for hand-eye — allows getting close with varied angles
            handeye_transform = self.estimate_pose_single_marker()
            if handeye_transform is not None:
                self.collect_handeye_sample(handeye_transform)

    def collect_handeye_sample(self, T_cam_keyboard: TransformStamped) -> None:
        '''Collect a (T_base_ee, T_cam_keyboard) pair for hand-eye calibration
        if the arm has moved sufficiently since the last sample.'''
        try:
            ee_tf = self.tf_buffer.lookup_transform(
                self.base_frame, self.ee_frame, rclpy.time.Time()
            )
        except Exception:
            return

        # Extract rotation and translation for the EE pose
        q_ee = ee_tf.transform.rotation
        R_base_ee = R.from_quat([q_ee.x, q_ee.y, q_ee.z, q_ee.w]).as_matrix()
        t_base_ee = np.array([
            [ee_tf.transform.translation.x],
            [ee_tf.transform.translation.y],
            [ee_tf.transform.translation.z]
        ], dtype=np.float64)

        # Check if arm has moved enough since last sample
        if self.last_ee_rotation is not None:
            angle_diff = np.arccos(np.clip(
                (np.trace(self.last_ee_rotation.T @ R_base_ee) - 1) / 2,
                -1.0, 1.0
            ))
            if angle_diff < MIN_ROTATION_DELTA:
                return
            self.get_logger().info(f"Arm rotated {np.degrees(angle_diff):.1f}° since last sample")

        self.last_ee_rotation = R_base_ee.copy()

        # Extract rotation and translation for the camera-to-keyboard transform
        q_cam = T_cam_keyboard.transform.rotation
        R_cam_target = R.from_quat([q_cam.x, q_cam.y, q_cam.z, q_cam.w]).as_matrix()
        t_cam_target = np.array([
            [T_cam_keyboard.transform.translation.x],
            [T_cam_keyboard.transform.translation.y],
            [T_cam_keyboard.transform.translation.z]
        ], dtype=np.float64)

        # Add to rolling buffer
        self.handeye_buffer.append((R_base_ee, t_base_ee, R_cam_target, t_cam_target))
        if len(self.handeye_buffer) > MAX_HANDEYE_SAMPLES:
            self.handeye_buffer.pop(0)

        self.get_logger().info(f"Hand-eye sample collected ({len(self.handeye_buffer)}/{MIN_HANDEYE_SAMPLES})")

        # Solve when we have enough samples
        if len(self.handeye_buffer) >= MIN_HANDEYE_SAMPLES:
            self.solve_handeye()

    def cache_urdf_transform(self) -> None:
        '''Cache the URDF's initial image_frame transform once available, then stop.'''
        if self.R_ee2cam_urdf is not None:
            return
        try:
            guessed_tf = self.tf_buffer.lookup_transform(
                self.ee_frame, self.camera_frame, rclpy.time.Time()
            )
        except Exception:
            return

        q_g = guessed_tf.transform.rotation
        self.R_ee2cam_urdf = R.from_quat([q_g.x, q_g.y, q_g.z, q_g.w]).as_matrix()
        self.t_ee2cam_urdf = np.array([
            [guessed_tf.transform.translation.x],
            [guessed_tf.transform.translation.y],
            [guessed_tf.transform.translation.z]
        ], dtype=np.float64)
        self.get_logger().info(
            f"Cached URDF image_frame transform: "
            f"xyz=({self.t_ee2cam_urdf[0][0]:.4f}, {self.t_ee2cam_urdf[1][0]:.4f}, {self.t_ee2cam_urdf[2][0]:.4f})"
        )

    def solve_handeye(self) -> None:
        '''Solve the hand-eye calibration AX=XB problem and publish the
        calibrated camera frame directly as ee_frame -> image_frame.

        Uses URDF xyz (translation) and calibrated rpy (rotation).
        Called every time a new sample is added, so it continuously
        improves as more observations are collected.'''
        if self.R_ee2cam_urdf is None:
            return

        R_gripper2base = [s[0] for s in self.handeye_buffer]
        t_gripper2base = [s[1] for s in self.handeye_buffer]
        R_target2cam = [s[2] for s in self.handeye_buffer]
        t_target2cam = [s[3] for s in self.handeye_buffer]

        try:
            # calibrateHandEye outputs T_cam2gripper (camera to EE)
            R_cam2ee, t_cam2ee = cv2.calibrateHandEye(
                R_gripper2base, t_gripper2base,
                R_target2cam, t_target2cam,
                method=cv2.CALIB_HAND_EYE_TSAI
            )
        except cv2.error as e:
            self.get_logger().warn(f"Hand-eye calibration failed: {e}")
            return

        # Validate result — if calibration failed, OpenCV returns near-identity
        angle = np.arccos(np.clip((np.trace(R_cam2ee) - 1) / 2, -1.0, 1.0))
        if angle < 0.01:  # near-identity means calibration didn't converge
            self.get_logger().warn(
                f"Hand-eye calibration returned identity ({len(self.handeye_buffer)} samples) "
                f"— need more distinct arm rotations"
            )
            return

        # Invert to get T_ee2cam_actual (EE to camera)
        R_ee2cam_actual = R_cam2ee.T

        # Store calibrated rotation for continuous publishing
        self.calibrated_quat = R.from_matrix(R_ee2cam_actual).as_quat()  # [x, y, z, w]
        self.handeye_calibrated = True

        urdf_rpy = R.from_matrix(self.R_ee2cam_urdf).as_euler('xyz', degrees=True)
        calibrated_rpy = R.from_matrix(R_ee2cam_actual).as_euler('xyz', degrees=True)
        self.get_logger().info(
            f"Hand-eye calibration updated ({len(self.handeye_buffer)} samples):\n"
            f"  URDF image_frame:       rpy=[{urdf_rpy[0]:.2f}, {urdf_rpy[1]:.2f}, {urdf_rpy[2]:.2f}]\n"
            f"  Calibrated image_frame: rpy=[{calibrated_rpy[0]:.2f}, {calibrated_rpy[1]:.2f}, {calibrated_rpy[2]:.2f}]\n"
            f"  Delta:                  rpy=[{calibrated_rpy[0]-urdf_rpy[0]:.2f}, {calibrated_rpy[1]-urdf_rpy[1]:.2f}, {calibrated_rpy[2]-urdf_rpy[2]:.2f}]"
        )

    def publish_calibrated_camera_tf(self) -> None:
        '''Continuously publish calibrated camera transform at high rate
        to override robot_state_publisher's static TF.'''
        if not self.handeye_calibrated or self.t_ee2cam_urdf is None:
            return

        tfs = TransformStamped()
        tfs.header.stamp = self.get_clock().now().to_msg()
        tfs.header.frame_id = self.ee_frame
        tfs.child_frame_id = self.camera_frame
        tfs.transform.translation.x = float(self.t_ee2cam_urdf[0][0])
        tfs.transform.translation.y = float(self.t_ee2cam_urdf[1][0])
        tfs.transform.translation.z = float(self.t_ee2cam_urdf[2][0])
        tfs.transform.rotation.x = self.calibrated_quat[0]
        tfs.transform.rotation.y = self.calibrated_quat[1]
        tfs.transform.rotation.z = self.calibrated_quat[2]
        tfs.transform.rotation.w = self.calibrated_quat[3]

        self.transform_broadcaster.sendTransform(tfs)

    def aruco_callback(self, detection: ArucoDetection) -> None:
        """Store latest ArUco detections"""
        self.aruco_detection = detection
        self.publish_analysis_tf()

    def camera_info_callback(self, msg: CameraInfo) -> None:
        """Update camera intrinsics from RealSense CameraInfo."""
        k = msg.k  # 3x3 row-major
        self.camera_matrix = np.array([
            [k[0], k[1], k[2]],
            [k[3], k[4], k[5]],
            [k[6], k[7], k[8]]
        ], dtype=np.float32)
        self.dist_coeffs = np.array(msg.d, dtype=np.float64)
        self.camera_resolution = (msg.width, msg.height)

    def depth_callback(self, msg: Image) -> None:
        """Store latest aligned depth image."""
        # RealSense depth is 16UC1 in millimetres
        self.depth_image = np.frombuffer(msg.data, dtype=np.uint16).reshape(msg.height, msg.width)

    def refine_with_depth(self, x: float, y: float, z: float) -> tuple[float, float, float]:
        """Replace solvePnP depth with actual depth sensor reading.
        Projects the aruco 3D point to a pixel, reads depth, and back-projects."""
        if self.depth_image is None or self.camera_matrix is None:
            return x, y, z
        fx = self.camera_matrix[0, 0]
        fy = self.camera_matrix[1, 1]
        cx = self.camera_matrix[0, 2]
        cy = self.camera_matrix[1, 2]

        # Project 3D point to pixel
        u = int(fx * x / z + cx)
        v = int(fy * y / z + cy)
        h, w = self.depth_image.shape
        if not (0 <= u < w and 0 <= v < h):
            return x, y, z

        # Sample depth in a small window for robustness
        r = 3
        patch = self.depth_image[max(0,v-r):min(h,v+r+1), max(0,u-r):min(w,u+r+1)]
        valid = patch[patch > 0]
        if len(valid) == 0:
            return x, y, z
        depth_z = float(np.median(valid)) / 1000.0  # mm to metres

        # Back-project with real depth
        x_new = (u - cx) * depth_z / fx
        y_new = (v - cy) * depth_z / fy
        return x_new, y_new, depth_z

    def get_aruco_corners(self) -> None | np.ndarray:
        """
        Return marker centre positions as [x,y,z] in order
        """
        marker_lookup = {marker.marker_id: marker for marker in self.aruco_detection.markers}
        if any(marker_id not in marker_lookup for marker_id in self.marker_ids):
            return None

        ordered_points = []
        for marker_id in self.marker_ids:
            marker = marker_lookup[marker_id]
            x, y, z = marker.pose.position.x, marker.pose.position.y, marker.pose.position.z
            if self.use_depth:
                x, y, z = self.refine_with_depth(x, y, z)
            ordered_points.append([x, y, z])

        return np.array(ordered_points, dtype=np.float32)
    
    def estimate_rigid_transform(self, src_points: np.ndarray, dst_points: np.ndarray):
        """
        https://en.wikipedia.org/wiki/Kabsch_algorithm
        Estimates the rigid transform to map src_points -> dst_points using Kabsch's
        Returns rotation matrix and translation vector such that dst_points = R @ src_points + t
        """

        # Make sure that the two sets of points have the same dimensions and at least 3 points
        if src_points.shape != dst_points.shape or src_points.shape[0] < 3:
            return None, None

        # Get the average position/centroids of each set of points A (source) and B (destination)
        src_centroid = np.mean(src_points, axis=0) # C_A
        dst_centroid = np.mean(dst_points, axis=0) # C_B

        # Center each centroid at the origin
        src_centered = src_points - src_centroid # A'
        dst_centered = dst_points - dst_centroid # B'

        # Calculate the covariance matrix H = (A')^T*(B') which rotates the source points
        # such that they are aligned with the destination points about the origin
        H = src_centered.T @ dst_centered

        # Use SVD to to get U and V^T such that H = U*S*V^T
        # U represents the orientation of the source points
        # V represents the orientation of the destination points
        # S represents scaling/stretching (not used, as this is a rigid transform)
        U, S, VT = np.linalg.svd(H)
        
        # Apply determinant to account for reflections
        if np.linalg.det(VT.T @ U.T) < 0:
            VT[2, :] *= -1

        # Calculate the rotation matrix R = V*U^T
        # U^T rotates the source points to align with the orientation of the standard x, y, z axes, then
        # VT rotates these points to align with the destination points
        R = VT.T @ U.T

        # Calculate the translation vector t = C_B - R*C_A
        # R*C_A rotates the source centroid to align with the destination orientation
        t = dst_centroid - R @ src_centroid

        return R.astype(np.float32), t.reshape(3, 1).astype(np.float32)
        
    def get_corners(self, detected_pts) -> None:
        if self.camera_matrix is None:
            return
        fx = self.camera_matrix[0, 0]
        fy = self.camera_matrix[1, 1]
        cx = self.camera_matrix[0, 2]
        cy = self.camera_matrix[1, 2]

        image_points = []
        for pt in detected_pts:
            u = fx * pt[0] / pt[2] + cx
            v = fy * pt[1] / pt[2] + cy
            image_points.append([u, v])

        kb_msg = KeyboardPoints()
        kb_msg.points = [int(i) for point in image_points for i in point]
        kb_msg.width, kb_msg.height = self.camera_resolution
        self.point_pub.publish(kb_msg)
        
    def estimate_pose_single_marker(self) -> None | TransformStamped:
        """Estimate keyboard pose from a single ArUco marker (handeye_marker_id).
        Uses the marker's pose + its known offset on the keyboard to
        compute T_cam_keyboard. Only used for hand-eye calibration sampling."""
        if self.aruco_detection is None:
            return None

        marker_lookup = {m.marker_id: m for m in self.aruco_detection.markers}

        if self.handeye_marker_id not in marker_lookup:
            return None

        # Get the marker and its index in marker_ids for the known position lookup
        marker = marker_lookup[self.handeye_marker_id]
        try:
            idx = self.marker_ids.index(self.handeye_marker_id)
        except ValueError:
            self.get_logger().warn(f"handeye_marker_id {self.handeye_marker_id} not in marker_ids")
            return None

        # T_cam_marker from the ArUco detector
        q = marker.pose.orientation
        R_cam_marker = R.from_quat([q.x, q.y, q.z, q.w]).as_matrix()
        mx, my, mz = marker.pose.position.x, marker.pose.position.y, marker.pose.position.z
        if self.use_depth:
            mx, my, mz = self.refine_with_depth(mx, my, mz)
        t_cam_marker = np.array([[mx], [my], [mz]], dtype=np.float64)

        # Marker's known position on the keyboard (in keyboard frame, meters)
        t_keyboard_marker = self.keyboard_points_m[idx].reshape(3, 1).astype(np.float64)

        # T_cam_keyboard = T_cam_marker * T_marker_keyboard
        # Apply yaw offset to account for marker orientation on keyboard
        R_cam_keyboard = R_cam_marker @ self.R_marker_to_keyboard
        t_cam_keyboard = t_cam_marker - R_cam_keyboard @ t_keyboard_marker

        quat = R.from_matrix(R_cam_keyboard).as_quat()

        t = TransformStamped()
        t.header.stamp = self.get_clock().now().to_msg()
        t.header.frame_id = self.camera_frame
        t.child_frame_id = self.keyboard_frame
        t.transform.translation.x = float(t_cam_keyboard[0][0])
        t.transform.translation.y = float(t_cam_keyboard[1][0])
        t.transform.translation.z = float(t_cam_keyboard[2][0])
        t.transform.rotation.x = quat[0]
        t.transform.rotation.y = quat[1]
        t.transform.rotation.z = quat[2]
        t.transform.rotation.w = quat[3]

        return t

    def estimate_pose(self) -> None | TransformStamped:
        """Estimate pose of keyboard and return its transform"""
        # Get the marker poses in order
        detected_points_m = self.get_aruco_corners()
        if detected_points_m is None:
            return None
        
        # Publish keyboard corner points to gui
        self.get_corners(detected_points_m)
        
        # Get the rotation matrix and translation vector
        rmat, tvec = self.estimate_rigid_transform(self.keyboard_points_m, detected_points_m)
        if rmat is None or tvec is None:
            return None
        
        # Flip keyboard normal if it points in same direction as keyboard->camera
        tvec_unit = tvec.flatten() / np.linalg.norm(tvec)
        if np.dot(rmat[:, 2], tvec_unit) < 0:
            rmat[:, 2] *= -1
            rmat[:, 1] *= -1
        
        # Convert to rotation matrix then to quaternion 
        rot = R.from_matrix(rmat)
        quat = rot.as_quat()  # [x, y, z, w]
        ## convert quaternion and transform vector to transform message
        t = TransformStamped()
        t.header.stamp = self.get_clock().now().to_msg()
        t.header.frame_id = self.camera_frame
        t.child_frame_id = self.keyboard_frame

        t.transform.translation.x = float(tvec[0][0])
        t.transform.translation.y = float(tvec[1][0])
        t.transform.translation.z = float(tvec[2][0])

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