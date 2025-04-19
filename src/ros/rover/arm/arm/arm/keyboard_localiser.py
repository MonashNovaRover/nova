#!/usr/bin/env python
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

ROS service which when queried with a key, 
    returns its position on the keyboard.
Used for auto typing task at URC
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
 - Make a rectangle overlay on camera in GUI
 - Make the OPENCV auto aligner (for more complex solution)
   - Basic logic done
   - Determine how to make arm move
   - Test!
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from geometry_msgs.msg import PointStamped, TransformStamped
from arm_interfaces.srv import KeyPosition, StringTrigger

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data

from tf2_ros import Buffer, TransformListener, TransformBroadcaster

import numpy as np
import cv2
from cv_bridge import CvBridge

# Expected keyboard size:
KEYBOARD = (123, 354, 37) # (L, W, H) in mm 
# where L is length (column of keys direction [qaz]), W is width (row of keys direction [qwertyuiop]), H is height (From base to key)
# Hard coded key coordinates in mm (relative to center of keyboard; looking down at keyboard with cable facing up) 
# left = -x, right = +x, up = +y, down = -y 
# "key": (x, y) 
KEY_MAP = { 
    "esc": (-164, 50), "f1": (-126, 50), "f2": (-107, 50), "f3": (-88, 50), "f4": (-69, 50), "f5": (-40, 50), "f6": (-21, 50), "f7": (-2, 50), "f8": (17, 50), "f9": (46, 50), "f10": (65, 50), "f11": (84, 50), "f12": (103, 50), "prtsc": (126, 50), "scrlk": (145, 50), "pause": (164, 50),
    "`": (-164, 28), "1": (-145, 28), "2": (-126, 28), "3": (-107, 28), "4": (-88, 28), "5": (-69, 28), "6": (-50, 28), "7": (-31, 28), "8": (-12, 28), "9": (7, 28), "0": (26, 28), "-": (45, 28), "=": (64, 28), "backspace": (93, 28), "ins": (126, 28), "home": (145, 28), "pgup": (164, 28),
    "tab": (-159, 9), "q": (-135, 9), "w": (-116, 9), "e": (-97, 9), "r": (-78, 9), "t": (-59, 9), "y": (-40, 9), "u": (-21, 9), "i": (-2, 9), "o": (17, 9), "p": (36, 9), "[": (55, 9), "]": (74, 9), "\\": (97, 9), "del": (126, 9), "end": (145, 9), "pgdn": (164, 9),
    "capslock": (-157, -10), "a": (-130, -10), "s": (-111, -10), "d": (-92, -10), "f": (-73, -10), "g": (-54, -10), "h": (-35, -10), "j": (-16, -10), "k": (3, -10), "l": (22, -10), ";": (41, -10), "'": (60, -10), "enter": (91, -10),
    "lshift": (-152, -29), "z": (-121, -29), "x": (-102, -29), "c": (-83, -29), "v": (-64, -29), "b": (-45, -29), "n": (-26, -29), "m": (-7, -29), ",": (12, -29), ".": (31, -29), "/": (50, -29), "rshift": (86, -29), "uarrow": (145, -29),
    "lctrl": (-161, -48), "win": (-138, -48), "lalt": (-114, -48), "space": (-44, -48), "ralt": (29, -48), "fn": (53, -48), "menu": (76, -48), "rctrl": (101, -48), "larrow": (126, -48), "darrow": (145, -48), "rarrow": (164, -48) 
}

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
ros2 run arm keyboard_localiser.py

ros2 service call /get_key_position arm_interfaces/srv/KeyPosition '{key: "a"}'

In separate terminal:
- Run GUI
- Navigate to urc/auto-typing
"""

KEY_SERVICE_NAME = '/arm/keyboard/get_key_position'
KEYBOARD_TF_SERVICE_NAME = '/arm/keyboard/transform_toggle'


class KeyboardLocaliser(Node):
    def __init__(self):
        super().__init__('keyboard_localiser')

        # key position initalisation
        self.keyboard_frame = self.declare_parameter('keyboard_frame', 'keyboard_frame').get_parameter_value().string_value
        self.base_frame = self.declare_parameter('base_frame', 'base_link').get_parameter_value().string_value
        self.key_srv = self.create_service(KeyPosition, KEY_SERVICE_NAME, self.get_key_position_callback)
        self.key_map = KEY_MAP

        # keyboard alignment initalisation
        self.aligned_keyboard_position = self.declare_parameter('aligned_keyboard_position', DEFAULT_POSITION).get_parameter_value().double_array_value
        self.aligned_keyboard_quaternion = self.declare_parameter('aligned_keyboard_quaternion', DEFAULT_QUATERNION).get_parameter_value().double_array_value
        self.align_srv = self.create_service(StringTrigger, KEYBOARD_TF_SERVICE_NAME, self.get_align_callback)
        self.is_aligned = False

        # tf2 initalisation
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.transform_broadcaster = TransformBroadcaster(self)
        
        # timer to constantly publish tf at period/second
        timer_period = self.declare_parameter('tf_publish_rate', 1.0).get_parameter_value().double_value
        self.create_timer(timer_period, self.publish_tf)

        self.get_logger().info(f"Running this node with services: {KEY_SERVICE_NAME}, {KEYBOARD_TF_SERVICE_NAME}. Using keyboard: {self.keyboard_frame} for transforms and base link: {self.base_frame}")

    def get_key_position_callback(self, request, response):
        if request.key.lower() not in self.key_map:
            self.get_logger().warn(f"Key {request.key} not found in map.")
            return response

        x, y = self.key_map[request.key.lower()]
        pos = PointStamped()
        pos.header.frame_id = self.keyboard_frame # can add support for multiple keyboards by changing response depending on request header frame
        pos.header.stamp = request.header.stamp
        pos.point.x = x * 0.001 # convert mm to meters
        pos.point.y = y * 0.001
        pos.point.z = 0
        
        # attempt to transform position to base_link
        transformed = self.get_transform_to_base_link(pos)
        if transformed is None:
            self.get_logger().warn(f"Keyboard {self.keyboard_frame} transform could not be found.")
            return response

        response.position = transformed 
        self.get_logger().info(f"Returning request for {request.key}")
        return response

    def get_align_callback(self, request, response):
        """Once aligned, begin publishing TF"""
        self.get_logger().info(f"Toggling alignment for {request.value}")
        if request.value == "start":
            self.is_aligned = True
            response.message = f"Aligned. TF has begun publishing under {self.keyboard_frame} relative to {self.base_frame}"
        elif request.value == "stop":
            self.is_aligned = False
            response.message = f"TF publishing has been stopped"
        else:
            response.success = self.is_aligned
            response.message = f"\"{request.value}\" not recognised as a valid value."
            return response

        response.success = self.is_aligned
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

    def publish_tf(self) -> None:
        '''Publish the transform of the keyboard'''
        if not self.is_aligned:
            return
        
        tfs = TransformStamped()
        tfs.header.stamp = self.get_clock().now().to_msg()
        tfs.header.frame_id = self.base_frame
        tfs.child_frame_id = self.keyboard_frame
        tfs.transform.translation.x, tfs.transform.translation.y, tfs.transform.translation.z, = self.aligned_keyboard_position
        tfs.transform.rotation.x, tfs.transform.rotation.y, tfs.transform.rotation.z, tfs.transform.rotation.w = self.aligned_keyboard_quaternion

        self.transform_broadcaster.sendTransform(tfs)

class KeyboardPoseEstimation(Node):
    super().__init__('rectangle_aligner')

    PERISCOPE_IMAGE_TOPIC = '/arm/periscope'
    KEYBOARD_FRAME = 'keyboard_frame'
    CAMERA_FRAME = 'camera_frame'
    THRESHOLD_CONTOUR_AREA = (30000, 80000) # expected pixel area bounds of rectangle in image

    # Corner points of the keyboard relative to the keyboard frame in mm (center of keyboard)
    self.keyboard_points = np.array([
        [0, 0, 0],                      # top-left
        [KEYBOARD[1], 0, 0],            # top-right
        [KEYBOARD[1], KEYBOARD[0], 0],  # bottom-right
        [0, KEYBOARD[0], 0]             # bottom-left
    ], dtype=np.float32)

    # calibrated camera intrinsics TO BE CALIBRATED and determined for sim
    focal_length = 600  # px
    image_center = (320, 240)  # cx, cy

    self.camera_matrix = np.array([
        [focal_length, 0, image_center[0]],
        [0, focal_length, image_center[1]],
        [0, 0, 1]
    ], dtype=np.float32)
    # No distortion (or replace with your real ones)
    self.dist_coeffs = np.zeros(5)

    def __init__(self):
        self.view_sub = self.create_subscription(Image, PERISCOPE_IMAGE_TOPIC, self.view_callback, qos_profile=qos_profile_sensor_data)

    def view_callback(self, view):
        """ Callback when Image is recieved (Stores msg for retrieval) """
        self.view = view
    
    def msg_to_mat(self, logger, img, encoding):
        """Converts Image msg to cv2 frame"""
        mat = None
        try:
            mat = CvBridge().imgmsg_to_cv2(img, encoding)
        except Exception as e:
            logger.error(str(e))
        return mat

    def get_corners(self) -> np.array:
        """ Get the sorted corners of the keyboard from the image msg """
        image = self.msg_to_mat(self.get_logger(), self.view, 'bgr8')

        # Get contours
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        edges = cv2.Canny(gray, 50, 150)
        contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        # look for best rectangle contour
        best_rect = None
        for cnt in contours:
            # simplify contour polygon using algorithm (0.02 *cv2 arcLength is 2% of perimeter)
            approx = cv2.approxPolyDP(cnt, 0.02 * cv2.arcLength(cnt, True), True)
            if len(approx) == 4 and cv2.contourArea(approx) > threshold: # note: bigger rectangles may cause issues
                best_rect = approx
        
        # Extract and reshape points to 2D array
        corners = best_rect.reshape(4, 2)

        # Sort the points in order: top-left, top-right, bottom-right, bottom-left
        def sort_corners(pts):
            # pts: (4, 2)
            sorted_pts = np.zeros((4, 2), dtype="float32")

            s = pts.sum(axis=1)
            diff = np.diff(pts, axis=1)

            sorted_pts[0] = pts[np.argmin(s)]       # top-left
            sorted_pts[2] = pts[np.argmax(s)]       # bottom-right
            sorted_pts[1] = pts[np.argmin(diff)]    # top-right
            sorted_pts[3] = pts[np.argmax(diff)]    # bottom-left

            return sorted_pts

        image_points = sort_corners(corners)
        return image_points
        
    def estimate_pose(self) -> TransformStamped:
        ## run solvePnP
        image_points = self.get_corners()
        success, rvec, tvec = cv2.solvePnP(self.object_points, self.image_points, self.camera_matrix, self.dist_coeffs)
        ## convert rotation and transform vectors to transform message
       
        # Convert to rotation matrix then to quaternion
        rotation_matrix, _ = cv2.Rodrigues(rvec)
        rot = R.from_matrix(rotation_matrix)
        quat = rot.as_quat()  # [x, y, z, w]

        t = TransformStamped()
        t.header.stamp = self.get_clock().now().to_msg()
        t.header.frame_id = "keyboard"
        t.child_frame_id = "camera"

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


class RectangleAligner(Node):
    """
    Purpose of this node:
    - Realign the arm between each key press so its always accurate.
    - Can/Should be used when initally aligning the arm

    How it works:
    - Uses opencv to find the pose of the 2d rectangle, then will send adjustments to arm to align the rectangle

    Drawback:
    - Only considers 2D
    """
    super().__init__('rectangle_aligner')

    PERISCOPE_IMAGE_TOPIC = '/arm/periscope'
    THRESHOLD_CONTOUR_AREA = (30000, 80000) # expected pixel area bounds of rectangle in image
    IRL_TO_PIXEL_RATIO = 1                  # Used to convert rectangle to pixel expected width/height
    THRESHOLD_X = 5                         # maximum pixel devation from expected position
    THRESHOLD_Y = 5
    THRESHOLD_ANGLE = 2
    THRESHOLD_SCALE = 0.1


    def __init__(self):
        self.view_sub = self.create_subscription(Image, PERISCOPE_IMAGE_TOPIC, self.view_callback, qos_profile=qos_profile_sensor_data)

    def view_callback(self, view):
        """ Callback when Image is recieved (Stores msg for retrieval) """
        self.view = view
    
    def msg_to_mat(self, logger, img, encoding):
        """Converts Image msg to cv2 frame"""
        mat = None
        try:
            mat = CvBridge().imgmsg_to_cv2(img, encoding)
        except Exception as e:
            logger.error(str(e))
        return mat

    def alignment_check(self) -> None:
        """ Find rectangle in image, determine its properties and compared to desired alignment, then send pose to correct misalignment """
        image = self.msg_to_mat(self.get_logger(), self.view, 'bgr8')
        height, width, _ = image.shape
        expected_center = (width // 2, height // 2)
        expected_angle = 0
        expected_width, expected_height = KEYBOARD[1] * IRL_TO_PIXEL_RATIO, KEYBOARD[0] * IRL_TO_PIXEL_RATIO

        ### Find rectangle in image and determine its properties
        # Get contours
        gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        edges = cv2.Canny(gray, 50, 150)
        contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        # look for best rectangle contour
        best_rect = None
        for cnt in contours:
            # simplify contour polygon using algorithm (0.02 *cv2 arcLength is 2% of perimeter)
            approx = cv2.approxPolyDP(cnt, 0.02 * cv2.arcLength(cnt, True), True)
            if len(approx) == 4 and cv2.contourArea(approx) > threshold: # note: bigger rectangles may cause issues
                best_rect = approx
        
        rect = cv2.minAreaRect(best_rect)  # Gives (center, (width, height), angle)
        detected_center, (detected_width, detected_height), detected_angle = rect
        # If camera is rotated, normalize the angle
        if w < h:
            detected_angle = detected_angle + 90  
        self.get_logger().info(f"Center: {detected_center}, Size: {detected_width}x{detected_height}, Angle: {detected_angle}")

        ### Compare to desired alignment
        dx = detected_center[0] - expected_center[0]
        dy = detected_center[1] - expected_center[1]
        dtheta = detected_angle - expected_angle
        dscale = (detected_width * detected_height) / (expected_width * expected_height)

        ### Correct misalignment
        if abs(dx) > X_THRESHOLD:
            # correct for x
            pass
        elif abs(dy) > Y_THRESHOLD:
            # correct for y
            pass
        elif abs(dtheta) > ANGLE_THRESHOLD:
            # correct for angle
            pass
        elif abs(dscale - 1) > SCALE_THRESHOLD:
            # correct for scale
            pass
        else:
            # set aligned boolean to true
            pass
