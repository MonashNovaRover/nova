cmake_minimum_required(VERSION 3.16)
project(generic_broadcaster LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra)
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
)

find_package(ament_cmake REQUIRED)
find_package(backward_ros REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

generate_parameter_library(generic_broadcaster_parameters
        src/generic_broadcaster_parameter.yaml
)

add_library(generic_broadcaster SHARED
        src/generic_broadcaster.cpp
)
target_compile_features(generic_broadcaster PUBLIC cxx_std_17)
target_include_directories(generic_broadcaster PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/generic_broadcaster>
)
target_link_libraries(generic_broadcaster PUBLIC generic_broadcaster_parameters)
ament_target_dependencies(generic_broadcaster PUBLIC ${THIS_PACKAGE_INCLUDE_DEPENDS})
# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(generic_broadcaster PRIVATE "GENERIC_BROADCASTER_BUILDING_DLL")
pluginlib_export_plugin_description_file(controller_interface generic_broadcaster_plugin.xml)

install(
  DIRECTORY include/
  DESTINATION include/generic_broadcaster
)
install(TARGETS generic_broadcaster generic_broadcaster_parameters
  EXPORT export_generic_broadcaster
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_targets(export_generic_broadcaster HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
