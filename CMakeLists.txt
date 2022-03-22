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

# Find and include ROS2 dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
# Find and include message types dependencies
find_package(core REQUIRED)

# Add include directories for all targets
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)



################################################
# Add non-executable targets:
################################################

# Libraries:
# Gamepad
if (NOT TARGET gamepad)
  add_library(gamepad include/gamepad/gamepad.c)
  target_link_libraries(gamepad udev)
endif()

# CAN (socketcan)
if (NOT TARGET socketcan)
  add_library(socketcan include/socketcan/socketcan_cpp.cpp)
endif()

# Print functions
if (NOT TARGET print)
  add_library(print include/print/print.cpp)
endif()


# Other targets:
# CMDs
add_library(cmd src/cmd/cmd.cpp)
target_link_libraries(cmd socketcan print)



################################################
# Add executables:
################################################

# Inputs script
add_executable(inputs
  src/inputs/inputs_publisher.cpp
  src/inputs/joystick.cpp 
  src/inputs/joystick_gamepad.cpp 
  src/inputs/joystick_thrustmaster.cpp 
)
target_link_libraries(inputs gamepad print)
ament_target_dependencies(inputs rclcpp core)

# Drive Command script
add_executable(drive_inputs src/drive/drive_inputs.cpp)
target_link_libraries(drive_inputs print)
ament_target_dependencies(drive_inputs rclcpp core)

# Velocity Converter script
add_executable(vel_convert src/drive/velocity_convert.cpp)
target_link_libraries(vel_convert print)
ament_target_dependencies(vel_convert rclcpp core)

# Drive Receiver script
add_executable(driver
  src/drive/driver.cpp 
  src/drive/wheel.cpp
)
target_link_libraries(driver cmd print)
ament_target_dependencies(driver rclcpp core)

# PID Tuner script
add_executable(pid_tuner src/cmd/pid_tuner.cpp)
target_link_libraries(pid_tuner cmd print)
ament_target_dependencies(pid_tuner rclcpp core)


################################################
# Add Installations
################################################

# Add all the installation targets
install(TARGETS
  inputs
  drive_inputs
  driver
  vel_convert
  pid_tuner
  DESTINATION lib/${PROJECT_NAME}
)

# Add all Python executables to this list
install(PROGRAMS
  control/cmd_plotter.py
  DESTINATION lib/${PROJECT_NAME}
)



################################################
# Add Subdirectories
################################################

# Arm
add_subdirectory(src/arm)

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
