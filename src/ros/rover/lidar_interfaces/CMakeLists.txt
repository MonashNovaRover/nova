cmake_minimum_required(VERSION 3.5)
project(lidar_interfaces)

# Find dependencies
#find_package(ament_cmake REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Include all interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  # Message definitions
  "msg/jtop.msg"

  DEPENDENCIES std_msgs geometry_msgs nav_msgs action_msgs sensor_msgs
)

ament_package()