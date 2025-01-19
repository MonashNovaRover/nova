cmake_minimum_required(VERSION 3.8)
project(nova_behavior_tree)

set(lib_name ${PROJECT_NAME})

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
find_package(geometry_msgs REQUIRED)
find_package(vision_msgs REQUIRED)
find_package(aruco_opencv_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(nav2_util REQUIRED)
find_package(nav2_behavior_tree REQUIRED)
find_package(behaviortree_cpp REQUIRED)

set(dep_pkgs
  rclcpp
  std_srvs
  geometry_msgs
  vision_msgs
  aruco_opencv_msgs
  tf2_ros
  nav2_util
  nav2_behavior_tree
  behaviortree_cpp
)

# === Build ===

include_directories(
  include
)

add_library(nova_ar_tag_detected_condition_bt_node SHARED plugins/ar_tag_detected_condition.cpp)
list(APPEND plugin_libs nova_ar_tag_detected_condition_bt_node)

add_library(nova_compute_ar_tag_goals_action_bt_node SHARED plugins/compute_ar_tag_goals_action.cpp)
list(APPEND plugin_libs nova_compute_ar_tag_goals_action_bt_node)

add_library(nova_remove_passed_ar_tags_action_bt_node SHARED plugins/remove_passed_ar_tags_action.cpp)
list(APPEND plugin_libs nova_remove_passed_ar_tags_action_bt_node)

add_library(nova_save_image_service_bt_node SHARED plugins/save_image_service.cpp)
list(APPEND plugin_libs nova_save_image_service_bt_node)

add_library(nova_first_goal_reached_condition_bt_node SHARED plugins/first_goal_reached_condition.cpp)
list(APPEND plugin_libs nova_first_goal_reached_condition_bt_node)

add_library(nova_goals_empty_condition_bt_node SHARED plugins/goals_empty_condition.cpp)
list(APPEND plugin_libs nova_goals_empty_condition_bt_node)

add_library(nova_get_final_goal_condition_bt_node SHARED plugins/get_final_goal_condition.cpp)
list(APPEND plugin_libs nova_get_final_goal_condition_bt_node)

add_library(nova_get_first_goal_condition_bt_node SHARED plugins/get_first_goal_condition.cpp)
list(APPEND plugin_libs nova_get_first_goal_condition_bt_node)

add_library(nova_query_position_condition_bt_node SHARED plugins/query_position_condition.cpp)
list(APPEND plugin_libs nova_query_position_condition_bt_node)

add_library(nova_compute_search_goals_condition_bt_node SHARED plugins/compute_search_goals_condition.cpp)
list(APPEND plugin_libs nova_compute_search_goals_condition_bt_node)

add_library(nova_check_detected_condition_bt_node SHARED plugins/check_detected_condition.cpp)
list(APPEND plugin_libs nova_check_detected_condition_bt_node)

add_library(nova_led_service_bt_node SHARED plugins/led_service.cpp)
list(APPEND plugin_libs nova_led_service_bt_node)

foreach(bt_plugin ${plugin_libs})
  ament_target_dependencies(${bt_plugin} ${dep_pkgs})
  target_compile_definitions(${bt_plugin} PRIVATE BT_PLUGIN_EXPORT)
endforeach()


# === Installation ===

install(TARGETS ${plugin_libs}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY
  behavior_tree
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include/
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

ament_export_include_directories(
  include
)

ament_export_libraries(
  ${plugin_libs}
)

ament_export_dependencies(
  ${dep_pkgs}
)

ament_package()
