cmake_minimum_required(VERSION 3.5)
project(auto_start)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(DEPENDENCIES
  ament_cmake
  ament_cmake_python
  rclpy
  rclcpp
  nav_msgs
  geometry_msgs
  vision_msgs
  visualization_msgs
  std_msgs
  sensor_msgs
  tf2_ros
  tf2_msgs
)

foreach(DEPENDENCY IN ITEMS ${DEPENDENCIES})
  find_package(${DEPENDENCY} REQUIRED)
endforeach()

# Python executables
install(PROGRAMS
  auto_start/arch/start_auto_arch.py
  auto_start/urc/start_auto_urc.py
  DESTINATION lib/${PROJECT_NAME}
)

# Run build testing package nodes
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

ament_python_install_package(${PROJECT_NAME})
ament_package()
