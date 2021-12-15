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

# Find and include all dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(core REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Orocos_kdl REQUIRED)

# Library directories. Can import from these using <>
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  "${PROJECT_SOURCE_DIR}/include"
  #${Eigen3_INCLUDE_DIRS}  StackOverflow said this is required, but doesn't seem to be
)

# Add gamepad targets, as these are located in the include folder
if (NOT TARGET gamepad)
  add_library(gamepad SHARED include/gamepad/gamepad.c)
endif()

# Add CAN libraries
if (NOT TARGET socketcan)
  add_library(socketcan SHARED include/socketcan/socketcan_cpp.cpp)
endif()

# Add executables:

# Inputs script
add_executable(inputs src/inputs/inputs_publisher.cpp src/inputs/joystick.cpp 
  src/inputs/joystick_gamepad.cpp src/inputs/joystick_thrustmaster.cpp 
  ${PROJECT_SOURCE_DIR}/include/gamepad/gamepad.c)
target_link_libraries(inputs gamepad udev ${catkin_LIBRARIES})
ament_target_dependencies(inputs rclcpp core)

# Drive Command script
add_executable(drive_cmd src/drive/drive_publisher.cpp)
ament_target_dependencies(drive_cmd rclcpp core)

# Drive Receiver script
add_executable(driver src/drive/drive_subscriber.cpp src/drive/wheel.cpp src/cmd/cmd.cpp ${PROJECT_SOURCE_DIR}/include/socketcan/socketcan_cpp.cpp)
ament_target_dependencies(driver rclcpp core)

# Arm Publisher script
add_executable(arm_input src/arm/arm_publisher.cpp)
ament_target_dependencies(arm_input rclcpp core)

# Arm Driver script
add_executable(arm_driver src/arm/arm_driver.cpp src/arm/joint.cpp src/cmd/cmd.cpp ${PROJECT_SOURCE_DIR}/include/socketcan/socketcan_cpp.cpp)
ament_target_dependencies(arm_driver rclcpp core)

# Arm Kinematics script
add_executable(arm_model src/arm/kinematics/arm_model.cpp)
target_link_libraries(arm_model orocos-kdl)
ament_target_dependencies(arm_model rclcpp core)

# Add all the installation targets
install(TARGETS
  inputs
  drive_cmd
  driver
  arm_input
  arm_driver
  arm_model
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
