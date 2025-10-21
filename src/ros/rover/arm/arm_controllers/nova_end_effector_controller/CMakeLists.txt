cmake_minimum_required(VERSION 3.16)
project(nova_end_effector_controller LANGUAGES CXX)

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
  urdf
)

find_package(ament_cmake REQUIRED)
find_package(backward_ros REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

generate_parameter_library(nova_end_effector_controller_parameters
  src/nova_end_effector_controller_parameter.yaml
)

add_library(nova_end_effector_controller SHARED
  src/nova_end_effector_controller.cpp
  # src/self_collision_limiter.cpp
)
target_compile_features(nova_end_effector_controller PUBLIC cxx_std_17)
target_include_directories(nova_end_effector_controller PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/nova_end_effector_controller>
)
target_link_libraries(nova_end_effector_controller PUBLIC nova_end_effector_controller_parameters)
ament_target_dependencies(nova_end_effector_controller PUBLIC ${THIS_PACKAGE_INCLUDE_DEPENDS})
# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(nova_end_effector_controller PRIVATE "NOVA_end_effector_CONTROLLER_BUILDING_DLL")
# pluginlib_export_plugin_description_file(controller_interface end_effector_plugin.xml)

# install(DIRECTORY
#   launch
#   DESTINATION share/${PROJECT_NAME}
# )

install(
  DIRECTORY include/
  DESTINATION include/nova_end_effector_controller
)
install(TARGETS nova_end_effector_controller nova_end_effector_controller_parameters
  EXPORT export_nova_end_effector_controller
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_targets(export_nova_end_effector_controller HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
