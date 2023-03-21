#!/usr/bin/env python3
__package__ = 'autonomous'
import rclpy
from nav_msgs.msg import MapMetaData
from nav_msgs.msg import OccupancyGrid
from builtin_interfaces.msg import Time
from geometry_msgs.msg import Pose
from std_msgs.msg import Header
from rclpy.node import Node
import math


class GridPub(Node):

    def __init__(self):
        super().__init__("grid_pub")
        self.publisher = self.create_publisher(OccupancyGrid, "autonomous/occupancy_grid", 10)

    def publish_grid(self, resolution, width, height, x, y, theta, data):
        # This hold basic information about the characteristics of the OccupancyGrid
        """
        # The time at which the map was loaded
        time map_load_time
        # The map resolution [m/cell]
        float32 resolution
        # Map width [cells]
        uint32 width
        # Map height [cells]
        uint32 height
        # The origin of the map [m, m, rad].  This is the real-world pose of the
        # cell (0,0) in the map.
        geometry_msgs / Pose origin pass
        """

        meta_data = MapMetaData()
        meta_data.resolution = resolution
        meta_data.width = width
        meta_data.height = height

        meta_data.map_load_time = self.get_clock().now().to_msg()
        pose_origin = Pose()
        # pose_origin.position.x = 1 * self.width * 0.4/ 2
        # pose_origin.position.x = 1 * self.width * 0.4/ 2
        pose_origin.position.x = x
        pose_origin.position.y = y
        pose_origin.position.z = math.sin(theta / 2)
        pose_origin.orientation.w = math.cos(theta / 2)
        meta_data.origin = pose_origin

        header = Header()
        header.frame_id = 'map'   # map frame - this is important for tf2
        header.stamp = self.get_clock().now().to_msg()

        grid = OccupancyGrid()
        grid.header = header
        grid.info = meta_data
        grid.data = data

        self.get_logger().debug(f"Publishing occupancy grid: {grid}")

        self.publisher.publish(grid)


if __name__ == "__main__":
    rclpy.init()
    pub = GridPub()
    pub.publish_grid()
