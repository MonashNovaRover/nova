cmake_minimum_required(VERSION 3.16)
project(nova_twistmapper LANGUAGES CXX)

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
  tf2_geometry_msgs
)

find_package(ament_cmake REQUIRED)
find_package(backward_ros REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

include_directories(
	${EIGEN3_INCLUDE_DIRS}
)

generate_parameter_library(nova_twistmapper_parameters
  src/nova_twistmapper_parameter.yaml
)

add_library(nova_twistmapper SHARED
  src/PoseHandle.cpp
  src/nova_twistmapper.cpp
)
target_compile_features(nova_twistmapper PUBLIC cxx_std_17)
target_include_directories(nova_twistmapper PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/nova_twistmapper>
)
target_link_libraries(nova_twistmapper PUBLIC nova_twistmapper_parameters)
ament_target_dependencies(nova_twistmapper PUBLIC ${THIS_PACKAGE_INCLUDE_DEPENDS})
# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(nova_twistmapper PRIVATE "NOVA_TWISTMAPPER_BUILDING_DLL")
pluginlib_export_plugin_description_file(controller_interface twistmapper_plugin.xml)

install(DIRECTORY
  #launch
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include/nova_twistmapper
)
install(TARGETS nova_twistmapper nova_twistmapper_parameters
  EXPORT export_nova_twistmapper
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_targets(export_nova_twistmapper HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
