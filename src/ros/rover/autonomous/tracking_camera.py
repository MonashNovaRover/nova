import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry

import math
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs


"""

"""


class TrackingCamera(Node):
    """
    This object runs in a separate thread and either accepts input directly from the tracking camera, from a ROS node,
    or acts as a ROS-publisher. It maintains an internal state of the most recent tracking camera pose, can be
    configured to use wheel odometry, and
    """
    def __init__(self, mode="python", publish_to_ros=False):

        super().__init__("T265Node")

        # Declare RealSense pipeline, encapsulating the actual device and sensors
        self.pipe = rs.pipeline()

        self.camera_pub = self.create_publisher(Odometry, "/t265/odom/sample", 10)
        self.rover_pose_pub = self.create_publisher(Odometry, "/rover/pose", 10)

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_device("952322110473")
        self.cfg.enable_stream(rs.stream.pose)

        self.initial_x = 0.0
        self.initial_y = 0.0
        self.initial_yaw = 5.0 / 4.0 * math.pi

        self.pipe.start(self.cfg)

    def get_next_pose(self):
        frames = self.pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose:
            data = pose.get_pose_data()
            # calculate position - flip convert to correct x and y conventions

            msg = Odometry()
            msg.header.stamp = self.get_clock().now().to_msg()
            msg.header.frame_id = "map"

            # update translation
            msg.pose.pose.position.x = data.translation.x
            msg.pose.pose.position.y = data.translation.y
            msg.pose.pose.position.z = data.translation.z

            msg.pose.pose.orientation.x = data.rotation.x
            msg.pose.pose.orientation.y = data.rotation.y
            msg.pose.pose.orientation.z = data.rotation.z
            msg.pose.pose.orientation.w = data.rotation.w

            self.camera_pub.publish(msg)

            print("pose")


def main():
    rclpy.init()
    camera = TrackingCamera()
    for i in range(1000000):
        camera.get_next_pose()


if __name__ == "__main__":
    main()
