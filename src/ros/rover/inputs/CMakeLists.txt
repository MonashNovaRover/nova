cmake_minimum_required(VERSION 3.8)
project(inputs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(input_msgs REQUIRED)
find_package(rclcpp REQUIRED)

# Libraries:
# Gamepad
if (NOT TARGET gamepad)
  add_library(gamepad gamepad.c)
  target_link_libraries(gamepad udev)
endif()

if (NOT TARGET print)
  add_library(print print.cpp)
endif()

add_executable(inputs
  inputs_publisher.cpp
  joystick.cpp 
  joystick_gamepad.cpp 
  joystick_thrustmaster.cpp 
)

target_link_libraries(inputs gamepad print)
ament_target_dependencies(inputs rclcpp core SDL2)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
