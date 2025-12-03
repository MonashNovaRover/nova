cmake_minimum_required(VERSION 3.5)
project(science_interfaces)

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# uncomment the following section in order to fill in
# further dependencies manually.
# e.g.
# find_package(std_msgs REQUIRED)

#find_package(<dependency> REQUIRED)

# Include all interfaces
rosidl_generate_interfaces(${PROJECT_NAME}
  # Message definitions
#  "msg/Example.msg"

  # Service definitions


  # Action definitions


  # add any msg dependencies here
  # e.g. DEPENDENCIES std_msgs geometry_msgs
  DEPENDENCIES
)

ament_package()