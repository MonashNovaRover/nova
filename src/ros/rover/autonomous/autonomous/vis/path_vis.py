#!/usr/bin/python3
__package__ = "autonomous"

"""
Methods 
"""

import rclpy
from rclpy.node import Node
import autonomous.vis.pc_pub as pc_pub

from autonomous.config.ros_config import auto_waypoints_topic

from core.msg import Waypoints

# create custom msg type
from nav_msgs.msg import Path
from std_msgs.msg import Header
from geometry_msgs.msg import Point, Quaternion, Pose, PoseStamped

class PathCloud(Node):
    def __init__(self):
        
        super().__init__("path_vis_node")
        
        # create the path publisher
        self.path_publisher = self.create_publisher(Path, '/autonomous/path', 10) 
         
        self.subscriber_path = self.create_subscription(Waypoints, auto_waypoints_topic, self.path_callback, 10)
        self.path = Path()
    
    def path_callback(self, msg):
        self.construct_path(msg)
        self.publish_path()

    def construct_path(self, msg):
        """
        Contstructs a ros2 path message from a list of waypoints
        """
        header = Header()
        header.frame_id = 'local_map'
        path = Path()
        path.header = header

        for waypoint in msg.waypoints:
            pose_stamped = PoseStamped()
            pose_stamped.header = header

            point = Point()
            point.x, point.y, point.z = waypoint.x, waypoint.y, 0.0
            quat = Quaternion()
            
            pose = Pose()
            pose.position = point
            pose.orientation = quat

            pose_stamped.pose = pose            

            path.poses.append(pose_stamped)
        
        self.path = path

    def publish_path(self):
        """
        Given a path (which is an array of [x, y] coordinates), we want to publish a PointCloud2 object which shows straight lines for those paths
        """
        self.path_publisher.publish(self.path)
        self.get_logger().info("visualising_path")


def main(args=None):
    rclpy.init(args=args)
    node = PathCloud()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
    