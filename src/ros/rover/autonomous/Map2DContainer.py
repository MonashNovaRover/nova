from rclpy.node import Node
from core.msg import Map2D
from core.msg import Point2D

class Map2DContainer(Node):
  
  def __init__(self, is_listener=False, is_publisher=False):
    """
    is_listener == True => this is being used by path planner
    is_publisher == True => this is being created by the Map3D
    """

    self.grid = np.zeros()
    self.is_listener = is_listener
    
    if self.is_listener:
        self._subscriber = self.create_subscription(Map2D, "/2d_map_topic", self.grid_callback, 10)
    
    if self.is_publisher:
        self._publisher = self.create_publisher(Map2D, "/2d_map_topic", 10)


  def grid_callback(self, msg):
      self.grid = msg.points

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
