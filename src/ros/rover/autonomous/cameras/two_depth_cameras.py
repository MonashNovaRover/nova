import pyrealsense2.pyrealsense2 as rs
import numpy as np
import cv2
import logging
import rclpy
from config.runtime_params import d435_serial, d415_serial
from rclpy.logging import LoggingSeverity

class DepthCamera():
    def __init__(self, serial_number, publish_topic=None):
        self.serial_number = serial_number
        self.pipeline = rs.pipeline()
        self.config = rs.config()
        self.config.enable_device(self.serial_number)
        self.config.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 30)
        self.config.enable_stream(rs.stream.color, 640, 480, rs.format.bgr8, 30)

        self.pc = rs.pointcloud()
        self.decimate = rs.decimation_filter()
        self.decimate.set_option(rs.option.filter_magnitude, 2)
        self.colorizer = rs.colorizer()

    def start_pipeline(self):
        self.pipeline.start(self.config)

    def get_points(self):
        """
        Returning a new depth frame
 
        Coordinate Schema:
         :return: np.array(n, 6)
        """
 
         # Grab camera data
         # Wait for a coherent pair of frames: depth and color
         # t0 = time.time()
        frames = self.pipeline.wait_for_frames()
        depth_frame = frames.get_depth_frame()
        color_frame = frames.get_color_frame()
        depth_frame = self.decimate.process(depth_frame)

        # Grab new intrinsics (may be changed by decimation)
        # depth_intrinsics = rs.video_stream_profile(depth_frame.profile).get_intrinsics()

        color_image = np.asanyarray(color_frame.get_data())
        mapped_frame, color_source = color_frame, color_image

        points = self.pc.calculate(depth_frame)
        self.pc.map_to(mapped_frame)

        # Point-cloud data to arrays
        v, t = points.get_vertices(), points.get_texture_coordinates()
        verts = np.asanyarray(v).view(np.float32).reshape(-1, 3)  # xyz

        verts = verts[~((verts[:, 0] == 0) & (verts[:, 1] == 0) & (verts[:, 2] == 0))]

        verts = verts[~(verts[:, 2] > 4.5)]

        rclpy.logging._root_logger.log(
                f"Depth camera point cloud contained {len(verts)} points",
                LoggingSeverity.INFO,
                once=True,
                skip_first=True)
        if len(verts) < 10:
            rclpy.logging._root_logger.log(
                    f"Depth camera point cloud contained very few points",
                    LoggingSeverity.WARN,
                    once=False,
                    skip_first=True)
            if len(verts) == 0:
                rclpy.logging._root_logger.log(
                        f"Depth camera point cloud contained no points",
                        LoggingSeverity.ERROR,
                        once=False,
                        skip_first=True)
        
        return verts

    def print_points_len(self, points):
        print(f'number of points: {len(points)}, serial number: {self.serial_number}')

def main():
    
     d435 = DepthCamera(publish_topic = False, serial_number = d435_serial)
     print("connected d435")
     d415 = DepthCamera(publish_topic = False, serial_number = d415_serial)
     print("connected d415")
 
     d435.start_pipeline()
     print("started d415 pipeline")
     d415.start_pipeline()
     print("started d435 pipeline")
 
     while True:
         pts = d415.get_points()
         d415.print_points_len(pts)
 
         pts = d435.get_points()
         d435.print_points_len(pts)


if __name__ == "__main__":
    main()
