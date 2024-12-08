cmake_minimum_required(VERSION 3.5)
project(teleop_drive_joy LANGUAGES CXX)

set(THIS_PACKAGE_INCLUDE_DEPENDS
  control_msgs
  geometry_msgs
  sensor_msgs
  controller_manager_msgs
  drive_interfaces
  input_interfaces
  nova_interfaces
  pluginlib
  rclcpp
  rclcpp_components
  realtime_tools
  std_msgs
  std_srvs
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

# This library provides a way of loading parameters using a yaml
generate_parameter_library(
  teleop_drive_joy_parameters
  src/parameters.yaml
)

# This library provides a C++ interface for sending realtime twist or joint commands to a robot
add_library(${PROJECT_NAME} SHARED
  src/teleop_drive_joy.cpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${teleop_drive_joy_VERSION}")
target_link_libraries(${PROJECT_NAME} teleop_drive_joy_parameters)
ament_target_dependencies(${PROJECT_NAME} ${THIS_PACKAGE_INCLUDE_DEPENDS})

# Create a separate library for the plugin
add_library(teleop_drive_joy_plugin SHARED src/teleop_drive_joy.cpp)
set_target_properties(teleop_drive_joy_plugin PROPERTIES VERSION "${teleop_drive_joy_VERSION}")
target_link_libraries(teleop_drive_joy_plugin ${PROJECT_NAME})
ament_target_dependencies(teleop_drive_joy_plugin ${THIS_PACKAGE_INCLUDE_DEPENDS})

# Register the plugin
rclcpp_components_register_node(
  teleop_drive_joy_plugin
  PLUGIN "teleop_drive_joy::TeleopDriveJoy"
  EXECUTABLE teleop_drive_joy_node
)

#############
## Install ##
#############
# Install Libraries
install(
  TARGETS
    ${PROJECT_NAME}
    teleop_drive_joy_plugin
    teleop_drive_joy_parameters
  EXPORT teleop_drive_joyTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include/${PROJECT_NAME}
)