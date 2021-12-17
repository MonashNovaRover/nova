from rclpy.node import Node
from core.msg import Map2D
from core.msg import Point2D
import numpy as np


class Map2DContainer(Node):
    def __init__(self, is_publisher=False, is_ros=False, length=None, width=None):
        """
        is_publisher == True => this is being created by the Map3D

        A Map2D container holds a 2D cross section of the environment, and is useful for

        A 1-1 representation between coordinates on the ros topic autonomous/2d_map

        """

        super().__init__("Map2D" + "publisher" if is_publisher else "subscriber")
        
        self.grid = np.zeros((10, 10))
        self.is_publisher = is_publisher
        self.is_ros = is_ros
        
        self.length = length
        self.width = width

        if is_ros:
            if not self.is_publisher:
                self._subscriber = self.create_subscription(Map2D, "autonomous/map2d", self.grid_callback, 10)
            else:
                self._publisher = self.create_publisher(Map2D, "autonomous/map2d", 10)

    def grid_callback(self, msg):
        """
        Updates internal map the sparse points it reads from the ros topics
        """
        self.grid = np.zeros((msg.x_len, msg.y_len))

        xs = [point.x for point in msg.points]
        ys = [point.y for point in msg.points]

        for point in msg.points:
            self.grid[point.x, point.y] = 1
        self.resolution = True

    def update_from_3d(self, grid3d, pose):
        """
        Use this when creating map from DynamicMap to be published to ROS
        """
        # takes slice at a constant height, doesn't use pose
        height = 0
        resolution = 10
        sparsePointList = []
        maxX = 0
        maxY = 0

        # not sure how grid3d will be given, this assumes its a list of points
        # if we send an array thats indexed by its z value with a list of (x,y) points this would be a lot faster
        for point in grid3d:
            if point[2] == height:
                if point[0] > maxX:
                    maxX = point[0]
                if point[1] > maxY:
                    maxY = point[1]
                newPoint = Point2D()
                newPoint.x, newPoint.y = point[0], point[1]
                sparsePointList.append(newPoint)

        self.grid = sparsePointList
        newMap2D = Map2D()
        newMap2D.points = sparsePointList
        newMap2D.resolution = resolution
        newMap2D.x_len = maxX
        newMap2D.y_len = maxY
        self._publisher.publish(newMap2D)
