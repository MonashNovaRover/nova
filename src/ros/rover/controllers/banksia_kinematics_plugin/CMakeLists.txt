cmake_minimum_required(VERSION 3.16)
project(banksia_kinematics_plugin LANGUAGES CXX)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(THIS_PACKAGE_INCLUDE_DEPENDS
  geometry_msgs
  pluginlib
  rclcpp
  rcpputils
  tf2
  tf2_msgs
  tf2_geometry_msgs
  Eigen3
  tf2_eigen
  moveit_core
)

find_package(ament_cmake REQUIRED)
find_package(backward_ros REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

include_directories(
	${EIGEN3_INCLUDE_DIRS}
)

add_library(banksia_kinematics_plugin SHARED
  src/banksia_kinematics_plugin.cpp
)
target_compile_features(banksia_kinematics_plugin PUBLIC cxx_std_17)
target_include_directories(banksia_kinematics_plugin PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/banksia_kinematics_plugin>
)
ament_target_dependencies(banksia_kinematics_plugin PUBLIC ${THIS_PACKAGE_INCLUDE_DEPENDS})
# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(banksia_kinematics_plugin PRIVATE "BANKSIA_KINEMATICS_PLUGIN_BUILDING_DLL")
pluginlib_export_plugin_description_file(kinematics_base banksia_kinematics_plugin.xml)

install(DIRECTORY
  #launch
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include/banksia_kinematics_plugin
)
install(TARGETS banksia_kinematics_plugin
  EXPORT export_banksia_kinematics_plugin
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_targets(export_banksia_kinematics_plugin HAS_LIBRARY_TARGET)

ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
