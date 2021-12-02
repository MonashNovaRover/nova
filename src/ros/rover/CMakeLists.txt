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
find_package(rclcpp REQUIRED)
find_package(core REQUIRED)
include_directories(./include/)

# Add gamepad targets, as these are located in the include folder
if (NOT TARGET gamepad)
  add_library(gamepad SHARED include/gamepad/gamepad.c)
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
add_executable(driver src/drive/drive_subscriber.cpp src/drive/wheel.cpp)
ament_target_dependencies(driver rclcpp core)

# Arm Publisher script
add_executable(arm_input src/arm/arm_publisher.cpp)
ament_target_dependencies(arm_input rclcpp core)

# Add all the installation targets
install(TARGETS
  inputs
  drive_cmd
  driver
  arm_input
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
