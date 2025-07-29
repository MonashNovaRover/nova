cmake_minimum_required(VERSION 3.5)
project(generic_interfaces)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

# Include all interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  # Message definitions
  msg/Bool.msg
  msg/Byte.msg
  msg/Float64.msg
  msg/Int32.msg
  msg/String.msg

  # Service definitions
  srv/Bool.srv
  srv/Byte.srv
  srv/Float64.srv
  srv/Int32.srv
  srv/String.srv

  # Action definitions

  DEPENDENCIES std_msgs
)

ament_package()