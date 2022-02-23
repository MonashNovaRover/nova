__package__ = "autonomous"
import math
import time
import numpy as np
from threading import Thread
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs
from vis.pc_pub import PCPub
import rclpy
import cameras.artag_pose_detection as ar
import sys
from config.runtime_params import d415_serial


class DepthCamera(Thread):
    def __init__(self, callback, publish_topic=None, serial_number=d415_serial):
        super().__init__()
        if publish_topic:
            self.publisher = PCPub("depth_camera_pc_pub", scale=1)
        else:
            self.publisher = None

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
        w, h = self.depth_intrinsics.width, self.depth_intrinsics.height

        # Processing blocks
        self.pc = rs.pointcloud()
        self.decimate = rs.decimation_filter()
        self.decimate.set_option(rs.option.filter_magnitude, 2)
        self.colorizer = rs.colorizer()

    def run(self):
        while self.running:
            t = time.time()
            self.callback(self.get_points())
            sys.stdout.write("\r" + "Map update completed in: " + str(round(time.time() - t, 5)) + " seconds\n")
            sys.stdout.flush()

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


        t1 = time.time()
        depth_frame = frames.get_depth_frame()
        color_frame = frames.get_color_frame()

        depth_frame = self.decimate.process(depth_frame)

        # Grab new intrinsics (may be changed by decimation)
        depth_intrinsics = rs.video_stream_profile(depth_frame.profile).get_intrinsics()
        # depth_image = np.asanyarray(depth_frame.get_data())

        color_image = np.asanyarray(color_frame.get_data())

        # note: find better way of doing asynchronously

        # ar.findArTag(color_image)

        # depth_colormap = np.asanyarray(self.colorizer.colorize(depth_frame).get_data())

        mapped_frame, color_source = color_frame, color_image

        points = self.pc.calculate(depth_frame)
        self.pc.map_to(mapped_frame)

        # Point-cloud data to arrays
        v, t = points.get_vertices(), points.get_texture_coordinates()
        verts = np.asanyarray(v).view(np.float32).reshape(-1, 3)  # xyz

        verts = verts[~((verts[:, 0] == 0) & (verts[:, 1] == 0) & (verts[:, 2] == 0))]
        
        verts = verts[~(verts[:, 2] > 4.5)]
        
        # print("point processing: " + str(time.time() - t1))
        
        t2 = time.time()
        if self.publisher:
            pass
            # self.publisher.pub_pts_colors(verts, 255 * np.ones((verts.shape[0], 4)))
        # print("publishing: " + str(time.time() - t2))

        return verts

    def stop(self):
        # Stop streaming
        self.pipeline.stop()


def print_points_len(points):
    # print(points.shape)
    pass

def main():
    rclpy.init(args=None)
    camera = DepthCamera(print_points_len)
    camera.start()
    time.sleep(20)


if __name__ == "__main__":
    main()
