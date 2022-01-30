from rclpy.node import Node
import math
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs2


"""

"""


class TrackingCamera():
    """
    This object runs in a separate thread and either accepts input directly from the tracking camera, from a ROS node,
    or acts as a ROS-publisher. It maintains an internal state of the most recent tracking camera pose, can be
    configured to use wheel odometry, and
    """
    def __init__(self, mode="python", publish_to_ros=False):

        # Declare RealSense pipeline, encapsulating the actual device and sensors
        self.pipe = rs.pipeline()

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_stream(rs.stream.pose)

        self.initial_x = 0.0
        self.initial_y = 0.0
        self.initial_yaw = 5.0 / 4.0 * math.pi

        self.pipe.start(self.cfg)

    def get_next_pose(self):
        frames = self.pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose:
            self.data = pose.get_pose_data()
            # calculate position - flip convert to correct x and y conventions
            longitude = -self.data.translation.z + self.initial_y
            latitude = self.data.translation.x + self.initial_x

def main():
    camera = TrackingCamera()
    for i in range(1000000):
        camera.get_next_pose()
        print(str(round(camera.data.translation.z, 5)).rjust(10))


if __name__ == "__main__":
    main()
