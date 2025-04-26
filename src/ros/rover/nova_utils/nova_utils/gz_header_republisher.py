#!/usr/bin/env python3
# This node republishes a topic and changes the frame_id to whatever you set
# Currently used to set the header on /oak/points and /bootie/points to camera_link and bootie_link respectively in autobringup gazebo.launch.py
import rclpy
import importlib
from rclpy.node import Node
from rclpy.qos import QoSProfile

from std_msgs.msg import Header

class FrameFixer(Node):

    def __init__(self):
        super().__init__('frame_fixer')

        sub_topic = self.declare_parameter('sub', '/sub').get_parameter_value().string_value
        pub_topic = self.declare_parameter('pub', '/pub').get_parameter_value().string_value
        msg_type = self.declare_parameter('msg_type', 'msg_type').get_parameter_value().string_value
        self.target_frame = self.declare_parameter('target_frame', 'target_frame').get_parameter_value().string_value

        # Dynamically import message type
        message_module = importlib.import_module('.'.join(msg_type.split('/')[:-1]))     # e.g sensor_msgs/msg/Pointcloud2
        message_type = getattr(message_module, msg_type.split('/')[-1])     # from sensor_msgs.msg import PointCloud2
        
        # TODO check if message type has a header

        self.sub = self.create_subscription(message_type, sub_topic, self.repub, 10)
        self.pub = self.create_publisher(message_type, pub_topic, 10)

    def repub(self, msg):
        msg.header.frame_id = self.target_frame
        self.pub.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = FrameFixer()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()

