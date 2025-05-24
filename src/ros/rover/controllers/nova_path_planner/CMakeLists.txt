cmake_minimum_required(VERSION 3.16)
project(nova_path_planner LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(THIS_PACKAGE_INCLUDE_DEPENDS
  controller_interface
  generate_parameter_library
  geometry_msgs
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
  rcpputils
  realtime_tools
  tf2
  tf2_msgs
  nova_interfaces
  tf2_geometry_msgs
  moveit_core
  moveit_ros_planning
  moveit_ros_planning_interface
  urdf
)

find_package(ament_cmake REQUIRED)
find_package(backward_ros REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

include_directories(
)

generate_parameter_library(nova_path_planner_parameters
  src/nova_path_planner_parameter.yaml
)

add_library(nova_path_planner SHARED
  src/nova_path_planner.cpp
)
target_compile_features(nova_path_planner PUBLIC cxx_std_17)
target_include_directories(nova_path_planner PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/nova_path_planner>
)
target_link_libraries(nova_path_planner PUBLIC nova_path_planner_parameters)
ament_target_dependencies(nova_path_planner PUBLIC ${THIS_PACKAGE_INCLUDE_DEPENDS})
# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(nova_path_planner PRIVATE "NOVA_PATH_PLANNER_BUILDING_DLL")
pluginlib_export_plugin_description_file(controller_interface path_planner_plugin.xml)

install(DIRECTORY
  #launch
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include/nova_path_planner
)
install(TARGETS nova_path_planner nova_path_planner_parameters
  EXPORT export_nova_path_planner
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_targets(export_nova_path_planner HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
