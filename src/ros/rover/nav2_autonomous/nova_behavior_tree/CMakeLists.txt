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
find_package(ament_index_cpp REQUIRED)
find_package(visualization_msgs REQUIRED)

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
  ament_index_cpp
  visualization_msgs
)

# === Build ===

include_directories(
  include
)

# ========================================= ACTION NODES ============================================ #

add_library(nova_blackboard_publisher_bt_node SHARED plugins/action/blackboard_publisher_node.cpp)
list(APPEND plugin_libs nova_blackboard_publisher_bt_node)

add_library(nova_place_search_goals_action_bt_node SHARED plugins/action/place_search_goals_action.cpp)
list(APPEND plugin_libs nova_place_search_goals_action_bt_node)

add_library(nova_get_goal_action_bt_node SHARED plugins/action/get_goal_action.cpp)
list(APPEND plugin_libs nova_get_goal_action_bt_node)

add_library(nova_get_current_pose_action_bt_node SHARED plugins/action/get_current_pose_action.cpp)
list(APPEND plugin_libs nova_get_current_pose_action_bt_node)

add_library(nova_led_service_bt_node SHARED plugins/action/led_service.cpp)
list(APPEND plugin_libs nova_led_service_bt_node)

add_library(nova_remove_passed_urc_goals_action_bt_node SHARED plugins/action/remove_passed_urc_goals_action.cpp)
list(APPEND plugin_libs nova_remove_passed_urc_goals_action_bt_node)

add_library(nova_update_urc_goals_action_bt_node SHARED plugins/action/update_urc_goals_action.cpp)
list(APPEND plugin_libs nova_update_urc_goals_action_bt_node)

add_library(nova_snap_in_collision_goals_action_bt_node SHARED plugins/action/snap_in_collision_goals_action.cpp)
list(APPEND plugin_libs nova_snap_in_collision_goals_action_bt_node)

# ========================================= CONDITION NODES ========================================= #

add_library(nova_ar_tag_detected_condition_bt_node SHARED plugins/condition/ar_tag_detected_condition.cpp)
list(APPEND plugin_libs nova_ar_tag_detected_condition_bt_node)

add_library(nova_object_detected_condition_bt_node SHARED plugins/condition/object_detected_condition.cpp)
list(APPEND plugin_libs nova_object_detected_condition_bt_node)

add_library(nova_are_poses_near_condition_bt_node SHARED plugins/condition/are_poses_near_condition.cpp)
list(APPEND plugin_libs nova_are_poses_near_condition_bt_node)


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
  bt_nodes.xml
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
