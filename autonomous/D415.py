import pyrealsense2 as rs
from AppState import AppState

device = "D415"


class D415:
    """
    This class holds the functions and state variables used to interact with the camera and retrieve raw and pre-processed data
    """

    def __init__(self):
        self.state = AppState()

        # Configure depth and color streams
        self.pipeline = rs.pipeline()
        self.config = rs.config()

        if device == "D415":
            self.config.enable_stream(rs.stream.depth, 1280, 720, rs.format.z16, 30)
            self.config.enable_stream(rs.stream.color, 1280, 720, rs.format.bgr8, 30)

        # Start streaming
        self.pipeline.start(self.config)

        # Get stream profile and camera intrinsics
        self.profile = self.pipeline.get_active_profile()
        self.depth_profile = rs.video_stream_profile(self.profile.get_stream(rs.stream.depth))
        self.depth_intrinsics = self.depth_profile.get_intrinsics()

        # Processing blocks
        self.pc = rs.pointcloud()
        self.decimate = rs.decimation_filter()
        self.decimate.set_option(rs.option.filter_magnitude, 2 ** self.state.decimate)
        self.colorizer = rs.colorizer()

        # NOTE: we may need to call this each frame to fix some depth issues - experiment with this
        self.align_to = rs.stream.color
        self.align = rs.align(self.align_to)
        self.align_handler = rs.align(self.align_to)

