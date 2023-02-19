__package__ = "autonomous"
import time
import numpy as np
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs

import rclpy
from rclpy.node import Node
from std_msgs.msg import Header
from sensor_msgs.msg import PointCloud2, Image
from visualization_msgs.msg import ImageMarker

from autonomous.cameras.ar_tracker import ArTracker
from autonomous.cameras.pc_converter import create_cloud_xyz32
from cv_bridge import CvBridge
from autonomous.config.runtime_params import active_depth_camera


class DepthCamera(Node):
    def __init__(self):
        super().__init__("depth_camera")
        # Realsense processing filters and classes
        self.pc = rs.pointcloud()
        self.decimate = rs.decimation_filter(2)
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

        self.depth_frame = None
        self.color_frame = None
        self.latest_frame_stamp = None
        self.depth_frame_id = 'd435_1'

        self.ar_tracker = ArTracker(self.color_intrinsics, depth_cam_frame_id=self.depth_frame_id)
        # self.object_detector = ObjectDetector()
        self.cv_bridge : CvBridge = CvBridge()

        self.cloud_publisher = self.create_publisher(PointCloud2, f"~{self.depth_frame_id}/cloud", 10)
        self.image_publisher = self.create_publisher(Image, f"~{self.depth_frame_id}/image", 10)
        # RVIZ CANNOT DISPLAY IMAGE MARKERS. USEFUL IN FOXGLOVE
        # self.marker_publisher = self.create_publisher(ImageMarker, f"~{self.depth_frame_id}/markers", 10)

        self.timer_process_cloud = self.create_timer(1/self.param_pointcloud_frequency, self.process_pointcloud)
        self.timer_process_image = self.create_timer(1/self.param_image_frequency, self.process_image)

    def process_frames(self):
        """
        Returning a new depth frame

        Coordinate Schema:
        :return: np.array(n, 6)
        """
        # Wait for frames from camera
        frames = self.pipeline.wait_for_frames()
        self.latest_frame_stamp = self.get_clock().now().to_msg()

        # Align depth and color images from frames (so pixels match?)
        t1 = time.perf_counter()
        aligned = self.align.process(frames)
        t2 = time.perf_counter()
        self.depth_frame = aligned.get_depth_frame()
        self.color_frame = aligned.get_color_frame()
        t3 = time.perf_counter()


        self.get_logger().debug(f"frame alignment took {t2 - t1} s")
        self.get_logger().debug(f"getting frames took {t3 - t2} s")

    def process_image(self):
        color_image = np.asanyarray(self.color_frame.get_data())
        t1 = time.perf_counter()
        self.ar_tracker(color_image)
        t2 = time.perf_counter()
        # self.object_detector(self.color_frame, self.depth_frame)
        t3 = time.perf_counter()
        header = Header(
            stamp = self.latest_frame_stamp,
            frame_id = self.depth_frame_id
        )

        img_msg = self.cv_bridge.cv2_to_imgmsg(color_image, header=header)
        self.image_publisher.publish(img_msg)

        self.get_logger().debug(f"AR tag detection took {t2 - t1} s")
        self.get_logger().debug(f"object detection took {t3 - t2} s")

    def process_pointcloud(self):
        """
        Callback that converts depth frame into a pointcloud and publishes it
        """
        # Scale down depth frame
        t1 = time.perf_counter()
        processed_depth_frame = self.decimate.process(self.depth_frame)
        t2 = time.perf_counter()
        # Fill holes in depth frame
        processed_depth_frame = self.hole_filling.process(processed_depth_frame)
        t3 = time.perf_counter()

        points = self.pc.calculate(processed_depth_frame)
        t4 = time.perf_counter()

        # Point-cloud data to arrays
        v = points.get_vertices()
        verts = np.asanyarray(v).view(np.float32)

        # Do our own trimming? Not at the moment
        if False:
            verts = verts[~((verts[:, 0] == 0) & (verts[:, 1] == 0) & (verts[:, 2] == 0))]
            verts = verts[~(verts[:, 2] > 4.5)]

        header = Header(
            stamp = self.latest_frame_stamp,
            frame_id = self.depth_frame_id
        )

        pointcloud_msg = create_cloud_xyz32(header=header, points=verts) 
        self.cloud_publisher.publish(pointcloud_msg)
    
        # Log state of the pointcloud
        self.get_logger().debug(f"demication took {t2 - t1} s")
        self.get_logger().debug(f"hole filling took {t3 - t2} s")
        self.get_logger().debug(f"calculating pc took {t4 - t3} s")
        self.get_logger().debug(f"Depth camera point cloud contained {len(verts)} points")
        if len(verts) < 10:
            self.get_logger().warn(f"Depth camera point cloud contained < 10 points")
        elif len(verts) == 0:
            self.get_logger().error(f"Depth camera point cloud contained no points")

    def pub_colour(self):
        pass


def main():
    rclpy.init(args=None)
    camera = DepthCamera()
    rclpy.spin(camera)
    camera.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
