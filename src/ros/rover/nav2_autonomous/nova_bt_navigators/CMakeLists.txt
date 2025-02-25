cmake_minimum_required(VERSION 3.5)
project(nova_bt_navigators)

find_package(ament_cmake REQUIRED)
find_package(nav2_common REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_behavior_tree REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(std_srvs REQUIRED)
find_package(nav2_util REQUIRED)
find_package(nav2_core REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(pluginlib REQUIRED)
find_package(nova_auto_interfaces REQUIRED)
find_package(robot_localization REQUIRED)
find_package(geographic_msgs REQUIRED)

include_directories(
  include
)

set(lib_name ${PROJECT_NAME})

set(dependencies
  rclcpp
  rclcpp_action
  rclcpp_lifecycle
  rclcpp_components
  std_msgs
  geometry_msgs
  geographic_msgs
  robot_localization
  nav2_behavior_tree
  nav_msgs
  nav2_msgs
  behaviortree_cpp
  std_srvs
  nav2_util
  nav2_core
  tf2_ros
  pluginlib
  nova_auto_interfaces
)

add_library(${lib_name} SHARED 
  src/urc_navigator.cpp
)


install(TARGETS ${lib_name}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_target_dependencies(${lib_name} ${dependencies})
pluginlib_export_plugin_description_file(nav2_core nova_navigators.xml)
ament_export_dependencies(${dependencies})
ament_package()
