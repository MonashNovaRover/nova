__package__ = "autonomous"
#!/usr/bin/python3

"""
Methods 
"""

import rclpy
from rclpy.node import Node
import vis.pc_pub

from core.msg import Waypoints

# create custom msg type
# from core.msg import Path


class PathCloud(Node):
    def __init__(self):
        
        super().__init__("path_cloud")
        
        # create the point-cloud publisher (this is how we will visualise the rover)
        self.pc_pub = pc_pub.PCPub("path_cloud")
         
        self.subscriber_path = self.create_subscription(Waypoints, '/autonomous/goals', self.path_callback, 10)
    
    def path_callback(self, msg):
        path = [(pt.x, pt.y) for pt in msg.waypoints]
        self.publish_path(path)

    def publish_path(self, path):
        """
        Given a path (which is an array of [x, y] coordinates), we want to publish a PointCloud2 object which shows straight lines for those paths
        """
        pc = []
        for i in range(0, len(path) - 1):
            line = []

            n = int(100 * ((path[i][0] - path[i+1][0]) ** 2 + (path[i][1] - path[i+1][1]) ** 2) ** (1./2))
            for p in range(n):
                line.append((path[i][0] + (path[i+1][0]-path[i][0]) * (1/n) * p, (path[i][1] + (path[i+1][1] - path[i][1]) * (1/n) * p)))
            pc = pc + line
        
        pc = [[point[0], point[1], 0.0, 0, 0, 255, 0] for point in pc]

        self.pc_pub.pub(pc)
        
        print("visualizing path: ")

if __name__ == "__main__":
    rclpy.init(args=None)
    cloud = PathCloud()
    rclpy.spin(cloud)
    rclpy.shutdown()
    
