#!/usr/bin/python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This node ensures that a predefined list of required ROS 2 topics
is active before continuing execution. It is primarily used to
synchronize startup logic and ensure all sensor streams are
publishing before other nodes depend on them.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: topic_waiter
TOPICS:
  - /oak/rgbd/image_raw [sensor_msgs/msg/Image]
  - /bootie/rgbd/image_raw [sensor_msgs/msg/Image]
  - /oak/rgb/image_raw [sensor_msgs/msg/Image]
  - /oak/stereo/image_raw [sensor_msgs/msg/Image]
  - /oak/rgb/camera_info [sensor_msgs/msg/CameraInfo]
  - /bootie/rgb/image_raw [sensor_msgs/msg/Image]
  - /bootie/stereo/image_raw [sensor_msgs/msg/Image]
  - /bootie/rgb/camera_info [sensor_msgs/msg/CameraInfo]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):	Chetan Karthik Edupalli
CREATION:	26/09/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from rclpy.parameter import Parameter
import sys
import time


REQUIRED_TOPICS = [
    '/oak/rgbd/image_raw',
    '/bootie/rgbd/image_raw',
    '/oak/rgb/image_raw',
    '/oak/stereo/image_raw',
    '/oak/rgb/camera_info',
    '/bootie/rgb/image_raw',
    '/bootie/stereo/image_raw',
    '/bootie/rgb/camera_info',
]

CHECK_INTERVAL = 1.0  # seconds
TIMEOUT = 10  # seconds to give up


class TopicWaiter(Node):
    def __init__(self):
        super().__init__('topic_waiter')
        self.start_time = time.time()

    def all_topics_active(self):
        available_topics = [t[0] for t in self.get_topic_names_and_types()]
        return all(topic in available_topics for topic in REQUIRED_TOPICS)

    def spin_until_ready(self):
        self.get_logger().info(f"Waiting for required topics: {REQUIRED_TOPICS}")
        while rclpy.ok():
            rclpy.spin_once(self, timeout_sec=CHECK_INTERVAL)
            if self.all_topics_active():
                self.get_logger().info("All required topics are active.")
                return True
            if (time.time() - self.start_time) > TIMEOUT:
                self.get_logger().error("Timed out waiting for topics.")
                return False
            time.sleep(CHECK_INTERVAL)


def main(args=None):
    rclpy.init(args=args)
    node = TopicWaiter()

    success = node.spin_until_ready()
    node.destroy_node()
    rclpy.shutdown()

    if not success:
        sys.exit(1)


if __name__ == '__main__':
    main()
