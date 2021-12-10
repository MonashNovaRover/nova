#!/usr/bin/python3

"""
Methods 
"""

import numpy as np
import rclpy
from rclpy.node import Node
import PCPub
import transform

# create custom msg type
# from core.msg import Path

class PathCloud(Node):
    def __init__(self):
        
        super().__init__("path_cloud")
        
        # create the point-cloud publisher (this is how we will visualise the rover)
        self.pc_pub = PCPub.PCPub("path_cloud")
         
        # self.subscriber_path = self.create_subscription(Path, '/autonomous/path', self.callback, 10)
        
        self.publish_path([(0, 0), (0, 1), (1,2), (-3, 4)])

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
        
        pc = [[point[0], point[1], 0.5, 0, 0, 255, 0] for point in pc]

        self.pc_pub.pub(pc)

if __name__ == "__main__":
    rclpy.init(args=None)
    cloud = PathCloud()
    rclpy.spin(cloud)
    rclpy.shutdown()
    
