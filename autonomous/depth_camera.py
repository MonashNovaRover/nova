import math
import time
import numpy as np
import pyrealsense2 as rs
from threading import Thread


class AppState:
    def __init__(self, *args, **kwargs):
        self.WIN_NAME = 'RealSense'
        self.pitch, self.yaw = math.radians(-10), math.radians(-15)
        self.translation = np.array([0, 0, -1], dtype=np.float32)
        self.distance = 2
        self.prev_mouse = 0, 0
        self.mouse_btns = [False, False, False]
        self.paused = False
        self.decimate = 1
        self.scale = True
        self.color = True


class DepthCamera(Thread):
    def __init__(self, callback):
        super().__init__()
        self.state = AppState()

        self.running = True

        self.callback = callback

        # Configure depth and color streams
        self.pipeline = rs.pipeline()
        self.config = rs.config()

        self.pipeline_wrapper = rs.pipeline_wrapper(self.pipeline)
        self.pipeline_profile = self.config.resolve(self.pipeline_wrapper)
        self.device = self.pipeline_profile.get_device()

        self.config.enable_stream(rs.stream.depth, rs.format.z16, 30)
        self.config.enable_stream(rs.stream.color, rs.format.bgr8, 30)

        # Start streaming
        self.pipeline.start(self.config)

        # Get stream profile and camera intrinsics
        self.profile = self.pipeline.get_active_profile()
        self.depth_profile = rs.video_stream_profile(self.profile.get_stream(rs.stream.depth))
        self.depth_intrinsics = self.depth_profile.get_intrinsics()
        w, h = self.depth_intrinsics.width, self.depth_intrinsics.height

        # Processing blocks
        self.pc = rs.pointcloud()
        self.decimate = rs.decimation_filter()
        self.decimate.set_option(rs.option.filter_magnitude, 2 ** self.state.decimate)
        self.colorizer = rs.colorizer()
        out = np.empty((h, w, 3), dtype=np.uint8)

    def run(self):
        while self.running:
            self.callback(self.get_points())

    def get_points(self):
        """
        Returning a new depth frame

        Coordinate Schema:



        :return: np.array(n, 6)
        """

        # Grab camera data
        # Wait for a coherent pair of frames: depth and color
        frames = self.pipeline.wait_for_frames()

        depth_frame = frames.get_depth_frame()
        color_frame = frames.get_color_frame()

        depth_frame = self.decimate.process(depth_frame)

        # Grab new intrinsics (may be changed by decimation)
        depth_intrinsics = rs.video_stream_profile(depth_frame.profile).get_intrinsics()
        w, h = depth_intrinsics.width, depth_intrinsics.height

        depth_image = np.asanyarray(depth_frame.get_data())
        color_image = np.asanyarray(color_frame.get_data())

        depth_colormap = np.asanyarray(
            self.colorizer.colorize(depth_frame).get_data())

        if self.state.color:
            mapped_frame, color_source = color_frame, color_image
        else:
            mapped_frame, color_source = depth_frame, depth_colormap

        points = self.pc.calculate(depth_frame)
        self.pc.map_to(mapped_frame)

        # Point-cloud data to arrays
        v, t = points.get_vertices(), points.get_texture_coordinates()

        verts = np.asanyarray(v).view(np.float32).reshape(-1, 3)  # xyz
        texcoords = np.asanyarray(t).view(np.float32).reshape(-1, 2)  # uv

        return np.concatenate((verts, texcoords), axis=1)

    def stop(self):
        # Stop streaming
        self.pipeline.stop()
