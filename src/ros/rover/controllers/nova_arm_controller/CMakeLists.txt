cmake_minimum_required(VERSION 3.16)
project(nova_arm_controller LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(THIS_PACKAGE_INCLUDE_DEPENDS
  controller_interface
  generate_parameter_library
  geometry_msgs
  hardware_interface
  nav_msgs
  pluginlib
  rclcpp
  rclcpp_lifecycle
  rcpputils
  realtime_tools
  tf2
  tf2_msgs
  nova_interfaces
  joint_limits
)

find_package(ament_cmake REQUIRED)
find_package(backward_ros REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

generate_parameter_library(nova_arm_controller_parameters
  src/nova_arm_controller_parameter.yaml
)

add_library(nova_arm_controller SHARED
  src/nova_arm_controller.cpp
)
target_compile_features(nova_arm_controller PUBLIC cxx_std_17)
target_include_directories(nova_arm_controller PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/nova_arm_controller>
)
target_link_libraries(nova_arm_controller PUBLIC nova_arm_controller_parameters)
ament_target_dependencies(nova_arm_controller PUBLIC ${THIS_PACKAGE_INCLUDE_DEPENDS})
# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(nova_arm_controller PRIVATE "NOVA_ARM_CONTROLLER_BUILDING_DLL")
pluginlib_export_plugin_description_file(controller_interface arm_plugin.xml)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include/nova_arm_controller
)
install(TARGETS nova_arm_controller nova_arm_controller_parameters
  EXPORT export_nova_arm_controller
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_targets(export_nova_arm_controller HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
