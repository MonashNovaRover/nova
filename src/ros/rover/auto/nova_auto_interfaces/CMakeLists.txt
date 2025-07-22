cmake_minimum_required(VERSION 3.5)
project(nova_auto_interfaces)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(std_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(geographic_msgs REQUIRED)

# Find default generators
find_package(rosidl_default_generators REQUIRED)

# Include all interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  # Action definitions
  "action/URC2024Navigator.action"

  DEPENDENCIES std_msgs geometry_msgs nav_msgs geographic_msgs
)

ament_package()