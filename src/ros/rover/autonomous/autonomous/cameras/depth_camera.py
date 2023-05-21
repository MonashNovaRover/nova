#!/usr/bin/env python3
__package__ = "autonomous"
import time
import numpy as np
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs
import logging

import rclpy
from rclpy.node import Node
from rclpy.time import Time
from std_msgs.msg import Header
from sensor_msgs.msg import PointCloud2, Image
from rs2_ros2 import rs2_verts_to_buffer
from autonomous.cameras.pc_converter import get_fields_xyz32
from autonomous.object_detection.object_detection import ObjectDetection

from autonomous.cameras.ar_tracker import ArTracker
from cv_bridge import CvBridge
from autonomous.config.runtime_params import active_depth_camera


class DepthCamera(Node):
    def __init__(self):
        super().__init__("depth_camera")
        self.get_logger().set_level(logging.INFO)
        # Realsense processing filters and classes
        self.decimation_filters = self.initialise_decimators()
        self.decimation_index = 2
        self.param_target_cloud_processing_time = self.declare_parameter("target_processing_time_s", 0.2).value
        self.param_do_blocks = self.declare_parameter("do_blocks", False).value

        self.pc = rs.pointcloud()
        self.hole_filling = rs.hole_filling_filter()
        self.align = rs.align(rs.stream.color)
        # Configure depth and color streams
        self.pipeline = rs.pipeline()
        self.config = rs.config()

        self.serial_number = active_depth_camera
        self.config.enable_device(self.serial_number)

        # enable streams for depth and color
        self.config.enable_stream(rs.stream.depth, rs.format.z16, 30)
        self.config.enable_stream(rs.stream.color, rs.format.bgr8, 30)

        # Start streaming
        self.pipeline.start(self.config)

        # Get stream profile and camera intrinsics
        self.profile = self.pipeline.get_active_profile()
        self.color_profile = rs.video_stream_profile(self.profile.get_stream(rs.stream.color))
        self.color_intrinsics = self.color_profile.get_intrinsics()

        # Get depth intrinsics
        self.depth_profile = rs.video_stream_profile(self.profile.get_stream(rs.stream.depth))
        self.depth_intrinsics = self.depth_profile.get_intrinsics()

        self.depth_frame = None
        self.color_frame = None
        self.latest_frame_stamp = None
        self.depth_frame_id = 'd435_1'

        self.ar_tracker = ArTracker(self.color_intrinsics, depth_cam_frame_id=self.depth_frame_id)
        if self.param_do_blocks:
            self.object_detector = ObjectDetection(self.depth_intrinsics)
        self.cv_bridge : CvBridge = CvBridge()

        self.param_pointcloud_frequency = self.declare_parameter("depth_cloud_rate_hz", 5).value
        self.param_image_frequency = self.declare_parameter("image_process_rate_hz", 5).value
        self.param_frame_frequency = self.declare_parameter("depth_cam_frame_rate_hz", 5).value

        self.cloud_publisher = self.create_publisher(PointCloud2, f"~/{self.depth_frame_id}/cloud", 10)
        self.image_publisher = self.create_publisher(Image, f"~/{self.depth_frame_id}/image", 10)
        # RVIZ CANNOT DISPLAY IMAGE MARKERS. USEFUL IN FOXGLOVE
        # self.marker_publisher = self.create_publisher(ImageMarker, f"~{self.depth_frame_id}/markers", 10)

        self.timer_process_cloud = self.create_timer(1/self.param_pointcloud_frequency, self.process_pointcloud)
        self.timer_process_image = self.create_timer(1/self.param_image_frequency, self.process_image)
        self.timer_process_frames = self.create_timer(1/self.param_frame_frequency, self.process_frames)
        self.get_logger().info("Depth camera node up!")

    def to_stamp_message(self, stamp: float):
        """
        Convert a timestamp from the camera to a ROS message
        """
        time_s = int(stamp / 1000)
        time_ns = int((stamp / 1000 - time_s) * 1e9)
        return Time(seconds=time_s, nanoseconds=time_ns).to_msg()

    def initialise_decimators(self):
        """
        Adaptive decimation.
        We want to balance between performance and resolution of the pointcloud. This array allows us to
        dynamically adjust the level of decimation of the pointcloud according to the time taken to process
        the previous cloud
        """
        return [
            rs.decimation_filter(1),
            rs.decimation_filter(2),
            rs.decimation_filter(4),
            rs.decimation_filter(6),
            rs.decimation_filter(8),
        ]

    def adjust_decimation(self, time_taken_s):
        """
        Compare the time taken to process the last pointcloud with the target time.
        If the time is greater by a factor of more than two, increase the decimation.
        If it is less by a factor of more than two, reduce the decimation
        """
        target_ratio = time_taken_s / self.param_target_cloud_processing_time
        if target_ratio > 2 and self.decimation_index != len(self.decimation_filters) - 1:
            self.decimation_index += 1
        elif target_ratio < 0.5 and self.decimation_index != 0:
            self.decimation_index -= 1

    def process_frames(self):
        """
        Returning a new depth frame

        Coordinate Schema:
        :return: np.array(n, 6)
        """
        # Wait for frames from camera
        t0 = time.perf_counter()
        frames = self.pipeline.wait_for_frames()
        self.latest_frame_stamp = self.to_stamp_message(frames.get_timestamp())

        # Align depth and color images from frames (so pixels match?)
        t1 = time.perf_counter()
        aligned = self.align.process(frames)
        t2 = time.perf_counter()
        self.depth_frame = aligned.get_depth_frame()
        self.color_frame = aligned.get_color_frame()
        t3 = time.perf_counter()

        self.get_logger().debug(f"Waiting for frames took {t1 - t0} s")
        self.get_logger().debug(f"frame alignment took {t2 - t1} s")
        self.get_logger().debug(f"getting frames took {t3 - t2} s")

    def process_image(self):
        t0 = time.perf_counter()
        if self.color_frame is None:
            return
        color_image = np.asanyarray(self.color_frame.get_data())
        t1 = time.perf_counter()
        self.ar_tracker.find_ar_tags(color_image, self.latest_frame_stamp)
        t2 = time.perf_counter()
        if self.param_do_blocks:
            self.object_detector.object_detection(self.color_frame, self.depth_frame)
        t3 = time.perf_counter()
        header = Header(
            stamp = self.latest_frame_stamp,
            frame_id = self.depth_frame_id
        )

        img_msg = self.cv_bridge.cv2_to_imgmsg(color_image, header=header)
        self.image_publisher.publish(img_msg)
        t4 = time.perf_counter()

        self.get_logger().debug(f"Getting color image to array took {t1 - t0} s")
        self.get_logger().debug(f"AR tag detection took {t2 - t1} s")
        if self.param_do_blocks:
            self.get_logger().debug(f"object detection took {t3 - t2} s")
        self.get_logger().debug(f"Converting to image message took {t4 - t3} s")

    def process_pointcloud(self):
        """
        Callback that converts depth frame into a pointcloud and publishes it
        """
        if self.depth_frame is None:
            return
        header = Header()
        header.stamp = self.latest_frame_stamp
        header.frame_id = self.depth_frame_id

        # Scale down depth frame
        t1 = time.perf_counter()
        processed_depth_frame = self.decimation_filters[self.decimation_index].process(self.depth_frame)
        t2 = time.perf_counter()
        # Fill holes in depth frame
        processed_depth_frame = self.hole_filling.process(processed_depth_frame)
        t3 = time.perf_counter()

        points = self.pc.calculate(processed_depth_frame)
        t4 = time.perf_counter()

        # Point-cloud data to arrays
        v = points.get_vertices()
        verts : np.ndarray = np.asanyarray(v).view(np.float32).reshape((-1, 3))
        t5 = time.perf_counter()

        # Do our own trimming of nonsense data
        verts = verts[~((verts[:, 0] == 0) & (verts[:, 1] == 0) & (verts[:, 2] == 0))]
        verts = verts[~(verts[:, 2] > 6.0)]
        t6 = time.perf_counter()

        pointcloud_msg = self.get_pc_message(verts, header)
        t7 = time.perf_counter()
        self.cloud_publisher.publish(pointcloud_msg)
        t8 = time.perf_counter()

        pc_process_time = t8 - t3
        self.adjust_decimation(pc_process_time)

        # Log state of the pointcloud
        self.get_logger().debug(f"demication took {t2 - t1} s")
        self.get_logger().debug(f"hole filling took {t3 - t2} s")
        self.get_logger().debug(f"calculating pc took {t4 - t3} s")
        self.get_logger().debug(f"Converting points to np array took {t5 - t4} s")
        self.get_logger().debug(f"Numpy pointcloud trimming took {t6 - t5} s. Left with {len(verts)} points remaining")
        self.get_logger().debug(f"Converting points to pointcloud msg took {t7 - t6} s")
        self.get_logger().debug(f"Publishing pointcloud took {t8 - t7} s")
        self.get_logger().debug(f"Depth camera point cloud contained {len(verts)} points")
        if len(verts) < 10:
            self.get_logger().warn(f"Depth camera point cloud contained < 10 points")
        elif len(verts) == 0:
            self.get_logger().error(f"Depth camera point cloud contained no points")

    def get_pc_message(self, verts: np.ndarray, header: Header) -> PointCloud2:
        pc_msg = PointCloud2()
        pc_msg.header = header
        pc_msg.height = 1
        pc_msg.width = len(verts)
        pc_msg.is_dense = False
        pc_msg.is_bigendian = False
        pc_msg.point_step = 12
        pc_msg.row_step = pc_msg.point_step * pc_msg.width
        pc_msg.fields = get_fields_xyz32()
        pc_msg.data = rs2_verts_to_buffer(verts)
        return pc_msg

    #def pub_colour(self):
    #    pass


def main():
    rclpy.init(args=None)
    camera = DepthCamera()
    rclpy.spin(camera)
    camera.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()

