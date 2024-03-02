cmake_minimum_required(VERSION 3.8)
project(nova_behavior_tree)

set(lib_name nova_save_image_service_bt_node)

# === Environment ===

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

# === Dependencies ===
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_srvs REQUIRED)
find_package(pluginlib REQUIRED)
find_package(nav2_behavior_tree REQUIRED)
find_package(behaviortree_cpp REQUIRED)

set(dep_pkgs
  rclcpp
  std_srvs
  pluginlib
  nav2_behavior_tree
  behaviortree_cpp
)

# === Build ===

include_directories(
  include
)

add_library(${lib_name} SHARED 
  plugins/save_image_service.cpp
)

ament_target_dependencies(${lib_name} 
  ${dep_pkgs}
)

# === Installation ===

install(TARGETS ${lib_name})

install(DIRECTORY
  behavior_tree
  DESTINATION share/${PROJECT_NAME}
)

install(FILES
  nova_bt_nodes.xml
  DESTINATION share/${PROJECT_NAME}
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

ament_export_libraries(
  ${lib_name}
)

ament_export_dependencies(
  ${dep_pkgs}
)

ament_package()
