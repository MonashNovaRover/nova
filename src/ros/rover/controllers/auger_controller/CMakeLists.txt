cmake_minimum_required(VERSION 3.16)
project(auger_controller)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

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
    drive_interfaces
    input_interfaces
)

find_package(ament_cmake REQUIRED)
find_package(backward_ros REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

find_package(generate_parameter_library REQUIRED)

generate_parameter_library(auger_controller_parameters
  src/auger_controller_parameter.yaml
)

add_library(${PROJECT_NAME} SHARED
  src/auger_controller.cpp
)
target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(${PROJECT_NAME} ${THIS_PACKAGE_INCLUDE_DEPENDS})
target_link_libraries(${PROJECT_NAME}
  auger_controller_parameters
)
# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(${PROJECT_NAME} PRIVATE "auger_controller_BUILDING_DLL")
###pluginlib_export_plugin_description_file(controller_interface auger_controller.xml)

if(BUILD_TESTING)
  find_package(ament_cmake_gmock REQUIRED)
  find_package(controller_manager REQUIRED)
  #find_package(ros2_control_test_assets REQUIRED)

  ###ament_add_gmock(test_auger_controller
  ###  test/test_auger_controller.cpp
  ###  ENV config_file=${CMAKE_CURRENT_SOURCE_DIR}/test/config/test_auger_controller.yaml)
  ###target_link_libraries(test_auger_controller
  ###  auger_controller
  ###)
  ###ament_target_dependencies(test_auger_controller
  ###  geometry_msgs
  ###  hardware_interface
  ###  nav_msgs
  ###  rclcpp
  ###  rclcpp_lifecycle
  ###  realtime_tools
  ###  tf2
  ###  tf2_msgs
  ###  drive_interfaces
  ###)

  ###ament_add_gmock(test_load_auger_controller
  ###   test/test_load_auger_controller.cpp
  ###)
  ###ament_target_dependencies(test_load_auger_controller
  ###  controller_manager
  ###  #ros2_control_test_assets
  ###)
endif()

install(DIRECTORY include/
  DESTINATION include/auger_controller
)

install(TARGETS auger_controller auger_controller_parameters
  EXPORT export_auger_controller
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_targets(export_auger_controller HAS_LIBRARY_TARGET)

ament_export_dependencies(
  ${THIS_PACKAGE_INCLUDE_DEPENDS}
)

#ament_export_include_directories(
#  include
#)
#ament_export_libraries(
#  ${PROJECT_NAME}
#)
ament_package()

