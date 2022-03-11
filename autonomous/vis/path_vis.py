__package__ = "autonomous"
#!/usr/bin/python3

"""
Methods 
"""

import rclpy
from rclpy.node import Node
import vis.pc_pub as pc_pub

from config.ros_config import main_frame

from core.msg import Waypoints

# create custom msg type
from nav_msgs.msg import Path
from geometry_msgs.msg import Point, Quaternion, Pose, PoseStamped

class PathCloud(Node):
    def __init__(self):
        
        super().__init__("path_cloud_node")
        
        # create the path publisher
        self.path_publisher = self.create_publisher(Path, '/autonomous/path', 10) 
         
        self.subscriber_path = self.create_subscription(Waypoints, '/autonomous/goals', self.path_callback, 10)
        self.path = Path()
    
    def path_callback(self, msg):
        self.construct_path(msg)
        self.publish_path()

    def construct_path(self, msg):
        header = Header()
        header.frame_id = main_frame
        path = Path()
        path.header = header

        for waypoint in msg:
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

    def publish_path(self, path):
        """
        Given a path (which is an array of [x, y] coordinates), we want to publish a PointCloud2 object which shows straight lines for those paths
        """
        self.path_publisher.publish(self.path)
        self.get_logger().info("visualising_path")

if __name__ == "__main__":
    rclpy.init(args=None)
    cloud = PathCloud()
    rclpy.spin(cloud)
    rclpy.shutdown()
    
