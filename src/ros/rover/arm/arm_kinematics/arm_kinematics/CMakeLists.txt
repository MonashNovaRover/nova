cmake_minimum_required(VERSION 3.5)
project(arm_kinematics LANGUAGES CXX)

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
  transmission_interface
  fcl
)

find_package(ament_cmake REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

include_directories(
  include
)

# -- Libraries

# Add plugin implementations here!
add_library(${PROJECT_NAME} SHARED
  src/utilities.cpp
  src/joint_map/joint_map.cpp
  src/joint_map/joint_map_builder.cpp
  src/kinematics_plugins/kinematics_params.cpp
  src/kinematics_plugins/kinematics_base.cpp
  src/kinematics_plugins/forward_kinematics_plugin.cpp
  src/kinematics_plugins/inverse_kinematics_plugin.cpp
  src/collision/collision_plugin.cpp
  src/collision/fcl/fcl_collision_plugin.cpp
  src/collision/fcl/allowed_collision_matrix.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)  # Require C++17
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

ament_target_dependencies(${PROJECT_NAME} ${THIS_PACKAGE_INCLUDE_DEPENDS})

# Stuff for plugins
pluginlib_export_plugin_description_file(${PROJECT_NAME} plugins.xml)
install(FILES plugins.xml
  DESTINATION share/${PROJECT_NAME}
)

# Test cases
#if(TR)
find_package(ament_cmake_gtest REQUIRED)
#  find_package(ament_lint_auto REQUIRED)

# Add tests

#set(TEST_INPUTS_SOURCE_FILES
#  test/test_inputs.cpp
#  test/test_state.cpp
#  test/test_input_map.cpp
#  test/test_input_pipeline.cpp
#)
#ament_add_gtest(test_inputs
#  ${TEST_INPUTS_SOURCE_FILES}
#)
#target_link_libraries(test_inputs
#  ${PROJECT_NAME}
#)
#target_include_directories(test_inputs PUBLIC
#  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#)
#set_tests_properties(test_inputs
#  PROPERTIES
#  ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}"
#)

# Enable linting
#  ament_lint_auto_find_test_dependencies()
#endif()

# -- Install

# Install header files
install(
  DIRECTORY include/
  DESTINATION include
)

# Install config files
install(
  DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_libraries(
  ${PROJECT_NAME}
)

ament_export_targets(
  #  ${PROJECT_NAME}Targets
  export_${PROJECT_NAME}
  HAS_LIBRARY_TARGET
)

ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
