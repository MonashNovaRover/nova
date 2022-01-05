cmake_minimum_required(VERSION 3.5)
project(control)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(
  include
    ${catkin_INCLUDE_DIRS}
    "${PROJECT_SOURCE_DIR}/include"
)

# Find and include all dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)

find_package(core REQUIRED)
include_directories(./include/)

# Add gamepad targets, as these are located in the include folder
if (NOT TARGET gamepad)
  add_library(gamepad SHARED include/gamepad/gamepad.c)
endif()

# Add CAN libraries
if (NOT TARGET socketcan)
  add_library(socketcan SHARED include/socketcan/socketcan_cpp.cpp)
endif()





################################################
# Add executables:
################################################

# Inputs script
add_executable(inputs src/inputs/inputs_publisher.cpp src/inputs/joystick.cpp 
  src/inputs/joystick_gamepad.cpp 
  src/inputs/joystick_thrustmaster.cpp 
  ${PROJECT_SOURCE_DIR}/include/gamepad/gamepad.c
  ${PROJECT_SOURCE_DIR}/include/debug/print.cpp)
target_link_libraries(inputs gamepad udev ${catkin_LIBRARIES})
ament_target_dependencies(inputs rclcpp core)

# Drive Command script
add_executable(drive_inputs src/drive/drive_inputs.cpp
  ${PROJECT_SOURCE_DIR}/include/debug/print.cpp)
ament_target_dependencies(drive_inputs rclcpp core)

# Velocity Converter script
add_executable(vel_convert src/drive/velocity_convert.cpp
  ${PROJECT_SOURCE_DIR}/include/debug/print.cpp)
ament_target_dependencies(vel_convert rclcpp core)

# Drive Receiver script
add_executable(driver src/drive/driver.cpp 
  src/drive/wheel.cpp src/cmd/cmd.cpp 
  ${PROJECT_SOURCE_DIR}/include/socketcan/socketcan_cpp.cpp 
  ${PROJECT_SOURCE_DIR}/include/debug/print.cpp)
ament_target_dependencies(driver rclcpp core)

# Arm Publisher script
add_executable(arm_inputs src/arm/arm_inputs.cpp
  ${PROJECT_SOURCE_DIR}/include/debug/print.cpp)
ament_target_dependencies(arm_inputs rclcpp core)

# Arm Driver script
add_executable(arm_driver src/arm/arm_driver.cpp 
  src/arm/joint.cpp 
  src/cmd/cmd.cpp
  ${PROJECT_SOURCE_DIR}/include/socketcan/socketcan_cpp.cpp
  ${PROJECT_SOURCE_DIR}/include/debug/print.cpp)
ament_target_dependencies(arm_driver rclcpp core)

# PID Tuner script
add_executable(pid_tuner src/cmd/pid_tuner.cpp 
  src/cmd/cmd.cpp
  ${PROJECT_SOURCE_DIR}/include/socketcan/socketcan_cpp.cpp
  ${PROJECT_SOURCE_DIR}/include/debug/print.cpp)
ament_target_dependencies(pid_tuner rclcpp core)


################################################
# Add Installations
################################################

# Add all the installation targets
install(TARGETS
  inputs
  drive_inputs
  driver
  arm_inputs
  arm_driver
  vel_convert
  pid_tuner
  DESTINATION lib/${PROJECT_NAME}
)

# Add all the installation directories
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

# Add all Python executables to this list
install(PROGRAMS
  DESTINATION lib/${PROJECT_NAME}
)


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
