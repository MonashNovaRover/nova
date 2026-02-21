cmake_minimum_required(VERSION 3.5)
project(nova_utils)

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

set(THIS_PACKAGE_INCLUDE_DEPENDS
  ament_cmake
  rclpy
  rclcpp
  nav_msgs
  geometry_msgs
  vision_msgs
  visualization_msgs
  std_msgs
  sensor_msgs
  yolo_msgs
  tf2_ros
  tf2_msgs
)

foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

add_executable(transform_republisher src/transform_republisher.cpp)
ament_target_dependencies(transform_republisher ${THIS_PACKAGE_INCLUDE_DEPENDS})
target_include_directories(transform_republisher PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
  $<INSTALL_INTERFACE:include>)

# Python executables
install(PROGRAMS
  scripts/detection_overlay.py
  scripts/goal_marker.py
  scripts/gz_frame_fixer.py
  scripts/gz_gps_fixer.py
  scripts/gz_imu_fixer.py
  scripts/load_waypoint.py
  scripts/quaternion_to_euler.py
  scripts/record_metrics.py
  scripts/save_waypoint.py
  scripts/service_listener.py
  scripts/start_auto_2.py
  scripts/start_auto.py
  scripts/test_start_auto.py
  scripts/topic_waiter.py
  scripts/node_waiter.py

  DESTINATION lib/${PROJECT_NAME}
)

# C++ executables
install(TARGETS
  transform_republisher
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

ament_package()
