"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Localise cubes.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: cube_localisation
TOPICS:
  - subscriber: [Detection3DArray]
  - publisher: [MarkerArray]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_cube_localisation
AUTHOR(S):	Nova Rover
CREATION:	22/03/2024
EDITED:		24/03/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
  - Import vision_opencv
  - Import image_geometry
  - Grab/process pixel positions from ros topics
  - Find mean 3d point from all 3d points
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# ros imports
from typing import Union
import rclpy
import logging
import colorsys

from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
from tf2_ros import Buffer, TransformListener
from image_geometry import StereoCameraModel

# msg types
from visualization_msgs.msg import MarkerArray, Marker
from vision_msgs.msg import Detection2DArray, Detection2D
from sensor_msgs import Image, CameraInfo
from geometry_msgs.msg import PoseStamped, Pose, Transform, Pose2D
from std_msgs.msg import String, Empty, ColorRGBA

# standard python imports
from typing import Dict, List
import numpy as np
import time

IDEAL_VECTORS = {
    "RED": [1.0, 0.0, 0.0],
    "GREEN": [0.0, 1.0, 0.0],
    "BLUE": [0.0, 0.0, 1.0],
    "WHITE": [1.0, 1.0, 1.0],
} 

IDS_COLOR = {
    0: "RED",
    1: "GREEN",
    2: "BLUE",
    3: "WHITE",
}

COLOR_IDS = {
    "RED": 0,
    "GREEN": 1,
    "BLUE": 2,
    "WHITE": 3,
}

class CubeLocalisation(Node):
  """
  Takes a depth image (image where each pixel is assigned a depth value), finds the bounding box 
  around a cube spotted by the yolo model on the oakd camera, takes the pixels closest to the 
  box's centre, projects them onto a 3d point relative to the camera, takes the mean of those 
  points, converts the mean to be relative to the map, and publishes the pose as the location of 
  the localised cubes.
  """
  MIN_SAMPLES=5
  MAX_STD_DEV=0.2

  def __init__(self):
    super().__init__("cube_localisation")
    self.get_logger().set_level(logging.DEBUG)
    # ROS Subscribers
    self.sub_blocks = self.create_subscription(Detection2DArray, "/oak/nn/detections", self.cb_cube, 10)
    self.sub_depth_image = self.create_subscription(Image, "/oak/nn/detections", self.cb_depth_image, 10)
    self.sub_camera_info = self.create_subscription(CameraInfo, "/oak/stereo/camera_info", self.cb_camera_info, 10)

    # ROS publishers
    self.pub_confirmed_targets = self.create_publisher(MarkerArray, "~/localised_cubes", 10)

    # ROS Parameters
    # (to remove) self.param_desired_blocks = self.declare_parameter("tracked_block_colors", []).value
    self.param_max_reasonable_z = self.declare_parameter("maximum_target_z", 1.5).value
    self.param_min_reasonable_z = self.declare_parameter("minimum_target_z", -1.0).value
    self.param_map_coords_counterclockwise = self.declare_parameter("map_coords_cc", [10, 10, -10, 10, -10, -10, 10, -10]).value

    # (to remove) if self.param_desired_blocks is None:
    #     self.param_desired_blocks = []

    # ROS Tf2 stuff
    self.tf_buffer = Buffer()
    self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)
    
    self.map_xys_3d, self.map_edges = self.get_map_edges_from_boundary_points()
    self.state_rover_pose = Pose2D()

    # Internal variables
    self.last_blocks : MarkerArray = None
    self.new_blocks = False

    self.found_blocks = dict()

    self.unsure_blocks : Dict[List] = dict()
    
    timer_period = 0.1  # run the timer 10 times per second
    rover_pose_period = 1 / 30
    self.create_timer(timer_period, self.handle_targets)

    # (to keep) self.create_timer(rover_pose_period, self.callback_rover_pose)
    self.state_rover_pose.x = 0. # (to remove)
    self.state_rover_pose.y = 0. # (to remove)
    self.state_rover_pose.theta = 0. # (to remove)

  def get_map_edges_from_boundary_points(self):
    """
    Takes the map boundary points and returns a list of the edges of the map.
    Map boundary points are assumed to be provided in the counter-clockwise order.
    Returns a list of edges, where each edge is a 3D vector with zero z component, to be usable with cross product.
    """
    map_xys = np.array(self.param_map_coords_counterclockwise).reshape(-1, 2)
    map_xys_3d = np.hstack((map_xys, np.zeros((len(map_xys), 1))))
    map_edges = np.array([map_xys_3d[(i+1) % len(map_xys_3d)] - point for i, point in enumerate(map_xys_3d)])
    return map_xys_3d, map_edges

  def cb_cube(self, msg : Detection2DArray):
    """
    Callback for the bounding box topic. Receives a Detection2DArray of all detected blocks.
    """
    self.last_blocks = msg
    if len(msg.detections) > 0:
        self.new_blocks = True

  def cb_depth_image(self, msg : Image):
    """
    Callback for the depth image topic. Receives an Image from the camera.
    """
    self.depth_image = msg

  def cb_camera_info(self, msg : CameraInfo):
    """
    Callback for the camera info topic. Receives a CameraInfo from the camera.
    """
    self.camera_info = msg

  def to_map(self, cube_msg: Detection2D, img_msg: Image):
    """
    Converts a PoseStamped from the camera frame to the local map frame.

    Assumptions:
     - Index starts at 0
     - Bounding box center, x = row, y = column
    """
    try: 
      bounding_box = cube_msg.bbox
      camera = StereoCameraModel()

      # left_camera_info = self.camera_info
      # right_camera_info = self.camera_info
      # backup hardcoded CameraInfo msgs for L/R stereo cameras
      left_camera_info = CameraInfo()
      CameraInfo.header = self.camera_info.header
      CameraInfo.height = 1280
      CameraInfo.weight = 720
      CameraInfo.distortion_model = "plumb_bob"
      CameraInfo.D = [0.107416, -0.285425, -0.005807, -0.001530, 0.000000]
      CameraInfo.K = [800.40414,   0.     , 638.37063,
           0.     , 799.68286, 360.37479,
           0.     ,   0.     ,   1.     ]
      CameraInfo.R = [ 0.99961855, -0.02124419,  0.01764781,
          0.02115037,  0.99976125,  0.00548633,
         -0.01776014, -0.00511098,  0.99982921]
      CameraInfo.P = [833.04045,   0.     , 616.04111,   0.     ,
           0.     , 833.04045, 359.7206 ,   0.     ,
           0.     ,   0.     ,   1.     ,   0.     ]
      CameraInfo.binning_x = self.camera_info.binning_x
      CameraInfo.binning_y = self.camera_info.binning_y
      CameraInfo.roi = self.camera_info.roi
      right_camera_info = CameraInfo()
      CameraInfo.header = self.camera_info.header
      CameraInfo.height = 1280
      CameraInfo.weight = 720
      CameraInfo.distortion_model = "plumb_bob"
      CameraInfo.D = [0.027691, -0.084910, -0.002828, 0.001984, 0.000000]
      CameraInfo.K = [801.10787,   0.     , 661.27048,
           0.     , 802.27862, 362.70452,
           0.     ,   0.     ,   1.     ]
      CameraInfo.R = [ 0.99912007, -0.00732194,  0.04129749,
          0.00754073,  0.99995833, -0.00514455,
         -0.0412581 ,  0.00545144,  0.99913365]
      CameraInfo.P = [ 833.04045,    0.     ,  616.04111, -242.47655,
            0.     ,  833.04045,  359.7206 ,    0.     ,
            0.     ,    0.     ,    1.     ,    0.     ]
      CameraInfo.binning_x = self.camera_info.binning_x
      CameraInfo.binning_y = self.camera_info.binning_y
      CameraInfo.roi = self.camera_info.roi

      camera.fromCameraInfo(left_camera_info, right_camera_info)
      
      image = self.depth_image
      center_pixel_index = [bounding_box.center.x, bounding_box.center.y]
      pixel_indexes_to_project = [center_pixel_index]
      max_pixels_to_project = bounding_box.size_x * bounding_box.size_y
      available_pixels = (((bounding_box.size_x - 1)//2)*2) * (((bounding_box.size_y - 1)//2)*2)
      pixels_left = available_pixels - 1
      pixels_to_project = 1
      for i in range(0, 100):
        if pixels_to_project < max_pixels_to_project and pixels_left > 0:
          for j in range(0, i):
            if pixels_to_project < max_pixels_to_project and pixels_left > 0:
              x = -(i - 1) + j
              y = 1 + j
              try:
                if abs(x) > ((bounding_box.size_y - 1)//2) or abs(y) > ((bounding_box.size_x - 1)//2) or (center_pixel_index[0]+x) < 0 or center_pixel_index[1]+y < 0:
                  raise ValueError
                pixel_indexes_to_project.append((center_pixel_index[0]+x, center_pixel_index[1]+y))
                pixels_to_project += 1
                pixels_left -= 1
              except:
                pass
            else:
              break
          for j in range(0, i):
            if pixels_to_project < max_pixels_to_project and pixels_left > 0:
              x = 1 + j
              y = (i - 1) - j
              try:
                if abs(x) > ((bounding_box.size_y - 1)//2) or abs(y) > ((bounding_box.size_x - 1)//2) or (center_pixel_index[0]+x) < 0 or center_pixel_index[1]+y < 0:
                  raise ValueError
                pixel_indexes_to_project.append((center_pixel_index[0]+x, center_pixel_index[1]+y))
                pixels_to_project += 1
                pixels_left -= 1
              except:
                pass
            else:
              break
          for j in range(0, i):
            if pixels_to_project < max_pixels_to_project and pixels_left > 0:
              x = (i - 1) - j
              y = -(j + 1)
              try:
                if abs(x) > ((bounding_box.size_y - 1)//2) or abs(y) > ((bounding_box.size_x - 1)//2) or (center_pixel_index[0]+x) < 0 or center_pixel_index[1]+y < 0:
                  raise ValueError
                pixel_indexes_to_project.append((center_pixel_index[0]+x, center_pixel_index[1]+y))
                pixels_to_project += 1
                pixels_left -= 1
              except:
                pass
            else:
              break
          for j in range(0, i):
            if pixels_to_project < max_pixels_to_project and pixels_left > 0:
              x = -(j + 1)
              y = -(i - 1) + j
              try:
                if abs(x) > ((bounding_box.size_y - 1)//2) or abs(y) > ((bounding_box.size_x - 1)//2) or (center_pixel_index[0]+x) < 0 or center_pixel_index[1]+y < 0:
                  raise ValueError
                pixel_indexes_to_project.append((center_pixel_index[0]+x, center_pixel_index[1]+y))
                pixels_to_project += 1
                pixels_left -= 1
              except:
                pass
            else:
              break
        else:
          break

      projected_pixels = []
      for pixel_index in pixel_indexes_to_project:
        depth = image.data[(pixel_index[0]*image.step) + pixel_index[1]]
        disparity = camera.getDisparity(depth)
        projected_pixel = camera.projectPixelTo3d(pixel_index, disparity)
        projected_pixels.append(projected_pixel)

      projected_pixels_np = np.array(projected_pixels)
      mean_projected_pixel = projected_pixels_np.mean(axis=0)
      
      stamped_pose = PoseStamped()
      stamped_pose.header = cube_msg.header
      stamped_pose.pose.position.x = mean_projected_pixel[0]
      stamped_pose.pose.position.y = mean_projected_pixel[1]
      stamped_pose.pose.position.z = mean_projected_pixel[2]
      stamped_pose.pose.quaternion.x = 0.0
      stamped_pose.pose.quaternion.y = 0.0
      stamped_pose.pose.quaternion.z = 0.0
      stamped_pose.pose.quaternion.w = 0.0
      
      local_map_pose = self.tf_buffer.transform(stamped_pose, 'map')
      return local_map_pose
    except Exception as e:
      self.get_logger().warn(f"Error translating pose to local map frame: {e}")
      return None

  def remove_outlier_pos(self, pos_vals):
    """
    Removes any outlier positions from the list of positions. An outlier is defined as a position that is
    more than 3 standard deviations away from the mean.
    """
    mean = np.mean(pos_vals, axis=0)
    std_dev = np.std(pos_vals, axis=0)

    return [pos for pos in pos_vals if np.all(np.abs(pos - mean) < 2 * std_dev)]

  def attempt_confirm_target(self, color=None):
    """
    Checks that we have enough samples of this block, and that their position is sufficiently consistent
    to be considered a confirmed block.
    """
    target_pos = self.unsure_blocks[color]
    if len(target_pos) >= CubeLocalisation.MIN_SAMPLES:
      consistent_pos = self.remove_outlier_pos(target_pos)
    else:
      self.get_logger().debug(f"{len(target_pos)} samples is not enough to confirm target {color}")
      return

    if len(consistent_pos) >= CubeLocalisation.MIN_SAMPLES:
      self.get_logger().debug(f"Validating consistency of target {color}: {consistent_pos}")
      target_pos_vals = consistent_pos[-CubeLocalisation.MIN_SAMPLES:]
      # We have enough samples to be confident in this block's position
      # Calculate the average position of the block
      avg_pos = np.mean(target_pos_vals, axis=0)
      # Calculate the standard deviation of the block's position
      std_dev = np.std(target_pos_vals, axis=0)
      # Check that the standard deviation is small enough to be considered a confirmed block
      if np.all(std_dev < CubeLocalisation.MAX_STD_DEV):
        self.get_logger().debug(f"Confirmed target {color} consistent pos at {avg_pos}")
        # We have a confirmed block
        if color is not None:
          self.found_blocks[color] = avg_pos
          self.unsure_blocks.pop(color)
      else:
        self.get_logger().debug(f"Target {color} is not consistent enough: {consistent_pos}")

  def pose_in_map(self, pose: Pose) -> bool:
    """
    Checks if a pose is in the map by taking cross products with the edges of the map, in the counter-clockwise direction.
    """
    pos_vec = np.array([pose.position.x, pose.position.y, 0])
    self.get_logger().debug(f"Checking if pos {pose.position} is in map")
    for corner, edge in zip(self.map_xys_3d, self.map_edges):
      self.get_logger().debug(f"corner: {corner}, edge: {edge}, pos_vec: {pos_vec}")
      if np.cross(edge, pos_vec - corner)[2] < 0:
        # negative z component of cross product means the point is clockwise from the edge. This places it outside the edge and not in the map
        # Requires counter-clockwise ordering of edges
        return False

    return True

  def pose_not_reasonable(self, pose: Pose) -> bool:
    """
    Return true if the pose of an object is not reasonable for it to be in the map
    """
    not_reasonable = False

    if pose.position.z > self.param_max_reasonable_z or pose.position.z < self.param_min_reasonable_z:
      not_reasonable = True

    elif not self.pose_in_map(pose):
      not_reasonable = True

    return not_reasonable

  def update_blocks(self):
    """
    Updates the list of detected blocks, localising any new blocks and ignoring any blocks that have been
    found.
    """
    for block in self.last_blocks.detections:
      color = IDS_COLOR[block.results.id]
      if color in self.found_blocks: # (to remove) or color not in self.param_desired_blocks:
        # We already know where the block is, or we don't care about this block
        continue
      else:
        # We care about this block and haven't worked out where it is
        local_map_pose = self.to_map(block)
        if local_map_pose is None or self.pose_not_reasonable(local_map_pose):
          self.get_logger().debug(f"pose: {local_map_pose} not reasonable")
          continue
        # Append the block's pose to the list of estimated poses
        if color not in self.unsure_blocks:
          self.unsure_blocks[color] = []
        self.unsure_blocks[color].append([local_map_pose.position.x, local_map_pose.position.y])
        self.attempt_confirm_target(color=color)

    self.new_blocks = False

  def callback_rover_pose(self):
    """
    Stores the latest rover pose message into our State() variable.
    """
    try:
      base_link_tf : Transform = self.tf_buffer.lookup_transform("map", "base_link", Time()).transform
      self.get_logger().debug("Found transform from local_map to base_link", once=True)
    except:
      self.get_logger().warn("No transform from local_map to base_link", once=True)
    else:
      self.state_rover_pose.x = base_link_tf.translation.x
      self.state_rover_pose.y = base_link_tf.translation.y
      # self.state_rover_pose.theta = transform.quat_to_euler(base_link_tf.rotation)[2]
      q = base_link_tf.rotation
      t3 = 2 * (q.w*q.z + q.x*q.y)
      t4 = 1 - 2 * (q.y*q.y + q.z*q.z)
      yaw = np.arctan2(t3, t4)
      self.state_rover_pose.theta = yaw

  def publish_found(self):
    """
    Publishes the found blocks.
    """
    array = MarkerArray()
    for color, pos in self.found_blocks.items():
      msg = self.found_block_msg(color, pos)
      array.markers.append(msg)

    self.pub_confirmed_targets.publish(array)

  def found_block_msg(self, color_name: str, pos: List[float]) -> Marker:
    """
    Finalises a block.
    """
    msg = Marker()
    pose = Pose()
    pose.position.x = pos[0]
    pose.position.y = pos[1]
    pose.position.z = 0.0
    pose.orientation.w = 1.0
    msg.pose = pose
    msg.type = Marker.CUBE
    msg.scale.x = .1
    msg.scale.y = .1
    msg.scale.z = .1
    color = ColorRGBA()
    color.r = IDEAL_VECTORS[color_name][0]
    color.g = IDEAL_VECTORS[color_name][1]
    color.b = IDEAL_VECTORS[color_name][2]
    color.a = 1.
    msg.color = color
    msg.header.frame_id = "map"
    msg.header.stamp = self.get_clock().now().to_msg()
    # Namespace - raw messages can be separated from confirmed cubes
    msg.ns = "completed"
    msg.id = COLOR_IDS[color_name]
  
    return msg

  def handle_targets(self):
    if self.new_blocks:
      self.update_blocks()
    self.publish_found()



def main(args=None):
  rclpy.init(args=args)
  node = CubeLocalisation()
  rclpy.spin(node)
  node.destroy_node()
  rclpy.shutdown()


if __name__ == "__main__":
  main()
