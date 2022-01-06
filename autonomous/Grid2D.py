from rclpy.node import Node

class Grid2D(Node):
  
  def __init__(self, is_listener=False, is_publisher=False):
    """
    is_listener == True => this is being used by path planner
    is_publisher == True => this is being created by the Map3D
    """
    
    self.grid = np.zeros()
    self.is_listener = is_listener
    
    if self.is_listener:
        self._subscriber = self.create_subscription("/2d_map_topic")
    
    if self.is_publisher:
        self._publisher = self.create_publisher("/2d_map_topic")
    
  def update_from_3d(grid3d, pose):
      """
      Use this when creating map from DynamicMap to be published to ROS
      """
      self.grid = #update
