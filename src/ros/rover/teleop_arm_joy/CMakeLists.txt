cmake_minimum_required(VERSION 3.5)
project(teleop_arm_joy LANGUAGES CXX)

set(THIS_PACKAGE_INCLUDE_DEPENDS
  control_msgs
  geometry_msgs
  sensor_msgs
  controller_manager_msgs
  input_interfaces
  nova_interfaces
  pluginlib
  rclcpp
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

# Generate parameter library
generate_parameter_library(
  teleop_arm_joy_parameters
  src/parameters.yaml
)

# Build the node executable
add_executable(${PROJECT_NAME}_node
  src/teleop_arm_joy.cpp
  src/JoyDevice.cpp
  src/JoyButton.cpp
  src/JoyAxis.cpp
  src/control_modes/ControlMode.cpp
  src/control_modes/ControlModeManager.cpp
)

target_link_libraries(${PROJECT_NAME}_node
  teleop_arm_joy_parameters
)

ament_target_dependencies(${PROJECT_NAME}_node ${THIS_PACKAGE_INCLUDE_DEPENDS})

#############
## Install ##
#############

# Install the executable
install(
  TARGETS
    ${PROJECT_NAME}_node
    teleop_arm_joy_parameters
  DESTINATION lib/${PROJECT_NAME}
)

# Install header files
install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

# Install launch files
install(
  DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

# Install config files
install(
  DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()