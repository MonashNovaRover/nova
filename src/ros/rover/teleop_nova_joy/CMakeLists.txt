cmake_minimum_required(VERSION 3.5)
project(teleop_nova_joy LANGUAGES CXX)

set(THIS_PACKAGE_INCLUDE_DEPENDS
  control_msgs
  geometry_msgs
  sensor_msgs
  controller_manager_msgs
  core
  pluginlib
  rclcpp
  rclcpp_components
  realtime_tools
  std_msgs
  std_srvs
  tf2_eigen
)

find_package(ament_cmake REQUIRED)
find_package(generate_parameter_library REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

include_directories(
  include
)

###################
## C++ Libraries ##
###################

# This library provides a way of loading parameters for servo
generate_parameter_library(
  teleop_nova_joy_parameters
  config/xbox.config.yaml
)

# This library provides a C++ interface for sending realtime twist or joint commands to a robot
add_library(${PROJECT_NAME} SHARED
  src/teleop_nova_joy.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${teleop_nova_joy_VERSION}")
target_link_libraries(${PROJECT_NAME} teleop_nova_joy_parameters)
ament_target_dependencies(${PROJECT_NAME} ${THIS_PACKAGE_INCLUDE_DEPENDS})

add_library(teleop_nova_joy_ros SHARED src/teleop_nova_joy.cpp)
set_target_properties(teleop_nova_joy_ros PROPERTIES VERSION "${teleop_nova_joy_VERSION}")
target_link_libraries(teleop_nova_joy_ros ${PROJECT_NAME})
ament_target_dependencies(teleop_nova_joy_ros ${THIS_PACKAGE_INCLUDE_DEPENDS})

######################
## Components ##
######################

rclcpp_components_register_node(
  teleop_nova_joy_ros 
  PLUGIN "teleop_nova_joy::TeleopNovaJoy"
  EXECUTABLE teleop_nova_joy_node
)

#############
## Install ##
#############

# Install Libraries
install(
  TARGETS
    ${PROJECT_NAME}
    teleop_nova_joy_ros
    teleop_nova_joy_parameters
  EXPORT teleop_nova_joyTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include/${PROJECT_NAME}
)

# Install Binaries
install(
  TARGETS
  teleop_nova_joy
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

# Install include, launch, config directories
install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})

ament_export_targets(teleop_nova_joyTargets HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})

ament_package()
