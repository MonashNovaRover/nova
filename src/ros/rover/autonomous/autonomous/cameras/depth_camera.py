__package__ = "autonomous"
import time
import numpy as np
from threading import Thread
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs
from vis.pc_pub import PCPub
import rclpy
import sys
from cameras.ar_tracker import ArTracker
from config.runtime_params import active_depth_camera
from rclpy.logging import LoggingSeverity


class DepthCamera(Thread):
    def __init__(self, callback, publish_topic=None, serial_number=active_depth_camera):
        super().__init__()
        if publish_topic:
            self.publisher = PCPub("depth_camera_pc_pub", scale=1)
        else:
            self.publisher = None

        self.ar_tracker = ArTracker()

        self.running = True

        self.callback = callback

        # Configure depth and color streams
        self.pipeline = rs.pipeline()
        self.config = rs.config()

        self.serial_number = serial_number
        self.config.enable_device(self.serial_number)

        self.pipeline_wrapper = rs.pipeline_wrapper(self.pipeline)
        self.pipeline_profile = self.config.resolve(self.pipeline_wrapper)
        self.device = self.pipeline_profile.get_device()

        # enable streams for depth and color
        self.config.enable_stream(rs.stream.depth, rs.format.z16, 30)
        self.config.enable_stream(rs.stream.color, rs.format.bgr8, 30)

        # Start streaming
        self.pipeline.start(self.config)

        # Get stream profile and camera intrinsics
        self.profile = self.pipeline.get_active_profile()
        self.depth_profile = rs.video_stream_profile(self.profile.get_stream(rs.stream.depth))
        self.depth_intrinsics = self.depth_profile.get_intrinsics()
        # w, h = self.depth_intrinsics.width, self.depth_intrinsics.height

        # Processing blocks
        self.pc = rs.pointcloud()
        self.decimate = rs.decimation_filter()
        self.decimate.set_option(rs.option.filter_magnitude, 2)
        self.colorizer = rs.colorizer()

    def run(self):
        while self.running:
            t = time.time()
            # call the callback (probably 
            self.callback(self.get_points())
            rclpy.logging._root_logger.log(
                f"Map update completed in: {str(round(time.time() - t, 5))} seconds",
                LoggingSeverity.INFO,
                once=True,
                skip_first=True)

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
        self.ar_tracker(color_image)
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

        if self.publisher:
            pass

        return verts

    def stop(self):
        self.pipeline.stop()


def print_points_len(points):
    pass


def main():
    rclpy.init(args=None)
    camera = DepthCamera(print_points_len)
    camera.start()
    time.sleep(20)


if __name__ == "__main__":
    main()
