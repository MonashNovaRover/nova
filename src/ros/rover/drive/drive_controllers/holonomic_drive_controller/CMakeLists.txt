cmake_minimum_required(VERSION 3.16)
project(holonomic_drive_controller)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra)
endif()

set(THIS_PACKAGE_INCLUDE_DEPENDS
  controller_interface
  geometry_msgs
  hardware_interface
  nav_msgs
  std_srvs
  pluginlib
  rclcpp
  rclcpp_lifecycle
  rcpputils
  realtime_tools
  tf2
  tf2_msgs
  nova_controller_common
  nova_drive_controller_base
  Eigen3
)

find_package(ament_cmake REQUIRED)
find_package(backward_ros REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

find_package(generate_parameter_library REQUIRED)

generate_parameter_library(holonomic_drive_controller_parameters
        src/holonomic_drive_controller.yaml
)

add_library(${PROJECT_NAME} SHARED
        src/holonomic_drive_controller.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(${PROJECT_NAME} ${THIS_PACKAGE_INCLUDE_DEPENDS})
target_link_libraries(${PROJECT_NAME}
  holonomic_drive_controller_parameters
)
# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(${PROJECT_NAME} PRIVATE "holonomic_drive_controller_BUILDING_DLL")
pluginlib_export_plugin_description_file(controller_interface holonomic_drive_controller.xml)


install(
  DIRECTORY include/
  DESTINATION include/holonomic_drive_controller
)

install(TARGETS holonomic_drive_controller holonomic_drive_controller_parameters
  EXPORT export_holonomic_drive_controller
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_targets(export_holonomic_drive_controller HAS_LIBRARY_TARGET)

ament_export_dependencies(
  ${THIS_PACKAGE_INCLUDE_DEPENDS}
)

ament_package()

