#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Republish a topic and change the 
frame_id. Currently used to set the header on 
/oak/points and /bootie/points to camera_link and 
bootie_link respectively in auto_bringup 
gazebo.launch.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gz_frame_fixer
TOPICS:
  - subscriber: /sub    [dynamic]
  - publisher: /pub     [dynamic]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
EDITED BY:	Anthony Lew, Victor Bartlinski
CREATION:	27/04/2025
EDITED:		30/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Check if message type has a header
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import rclpy
import importlib
from rclpy.node import Node
from rclpy.qos import QoSProfile

from std_msgs.msg import Header

class FrameFixer(Node):

    def __init__(self):
        super().__init__('gz_frame_fixer')

        self.sub_topics = self.declare_parameter('sub', ['/sub']).get_parameter_value().string_array_value
        self.pub_topics = self.declare_parameter('pub', ['/pub']).get_parameter_value().string_array_value
        self.msg_types = self.declare_parameter('msg_type', ['msg_type']).get_parameter_value().string_array_value
        self.target_frames = self.declare_parameter('target_frame', ['frame_id']).get_parameter_value().string_array_value

        # Dynamically import message type
        self.subs = []
        self.pubs = []

        for i in range(len(self.msg_types)):
            sub_topic = self.sub_topics[i]
            pub_topic = self.pub_topics[i]
            msg_type = self.msg_types[i]

            message_module = importlib.import_module('.'.join(msg_type.split('/')[:-1]))    # e.g. sensor_msgs/msg/Pointcloud2 -> sensor_msgs.msg import PointCloud2
            message_type = getattr(message_module, msg_type.split('/')[-1])
            self.subs.append(self.create_subscription(message_type, sub_topic, self.sub_callback, 10))
            self.pubs.append(self.create_publisher(message_type, pub_topic, 10))

    def sub_callback(self, msg):
        msg_type = str(type(msg)).split("'")[1].split('.') # e.g. <class 'sensor_msgs.msg._point_cloud2.PointCloud2'> -> sensor_msgs/msg/Pointcloud2
        del msg_type[2]
        msg_type = '/'.join(msg_type)
        i = self.msg_types.index(msg_type)
        msg.header.frame_id = self.target_frames[i]
        self.get_logger().info(f"Republishing {self.sub_topics[i]} as {self.pub_topics[i]} with frame_id: {self.target_frames[i]}")
        self.pubs[i].publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = FrameFixer()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()

