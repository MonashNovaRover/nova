cmake_minimum_required(VERSION 3.8)
project(inputs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(input_interfaces REQUIRED)
find_package(rclcpp REQUIRED)
find_package(SDL2 REQUIRED)

include_directories(
  include
)

if (NOT TARGET gamepad)
  add_library(gamepad src/gamepad.c)
  target_link_libraries(gamepad udev)
endif()

if (NOT TARGET print)
  add_library(print src/print.cpp)
endif()

if (NOT TARGET keyboard)
  add_library(keyboard src/keyboard.cpp)
  target_link_libraries(keyboard print)
  ament_target_dependencies(keyboard SDL2 input_interfaces)
endif()

add_executable(inputs_publisher
  src/inputs_publisher.cpp
  src/joystick.cpp 
  src/joystick_gamepad.cpp 
  src/joystick_thrustmaster.cpp 
)

target_link_libraries(inputs_publisher gamepad print keyboard)
ament_target_dependencies(inputs_publisher input_interfaces rclcpp SDL2)

install(FILES 
        include/inputs/keyboard.h
        DESTINATION include/inputs)

install(TARGETS
  inputs_publisher
  keyboard
  DESTINATION lib/${PROJECT_NAME}
)

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
