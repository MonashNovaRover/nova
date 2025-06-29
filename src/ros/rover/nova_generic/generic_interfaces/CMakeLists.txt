cmake_minimum_required(VERSION 3.5)
project(nova_interfaces)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(std_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(sensor_msgs REQUIRED)

# Include all interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  # Message definitions

  # Service definitions

  # Action definitions

  DEPENDENCIES std_msgs geometry_msgs nav_msgs action_msgs sensor_msgs
)

ament_package()