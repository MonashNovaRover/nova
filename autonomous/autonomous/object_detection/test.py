from visualization_msgs.msg import Marker
from rclpy.node import Node
from std_msgs.msg import ColorRGBA
from std_msgs.msg import Header
from geometry_msgs.msg import Pose
import rclpy
from  geometry_msgs.msg import Vector3

import random


class NodePub(Node):
    def __init__(self):
        super().__init__("node1")
        self.pub = self.create_publisher(Marker, "markers", 10)
        self.header = Header()
        self.header.frame_id = 'map'   # map frame - this is important for tf2
        self.header.stamp = self.get_clock().now().to_msg()

rclpy.init()

p = NodePub()

while True:
    input()
    msg = Marker()
    pose = Pose()
    pose.position.y = random.random()
    pose.position.z = random.random()
    pose.position.x = random.random()
    print(pose.position)
    pose.orientation.w = 1.0
    msg.pose = pose
    msg.type = Marker.CUBE
    msg.scale.x = .1
    msg.scale.y = .1
    msg.scale.z = .1
    color = ColorRGBA()
    color.r = 255.0 / 255 
    color.g = 0. / 255.
    color.a = 255. / 255.
    color.b = 0. /255.
    msg.color = color
    msg.header = p.header
    p.pub.publish(msg)    

