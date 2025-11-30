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
  transmission_interface
  fcl
  kdl_parser
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
  src/utilities/utilities.cpp
  src/joint_map/joint_map.cpp
  src/joint_map/joint_map_builder.cpp
  src/common/kinematics_params.cpp
  src/common/kinematics_base.cpp
  src/forward/forward_kinematics_plugin.cpp
  src/inverse/inverse_kinematics_plugin.cpp
  src/collision/collision_plugin.cpp
  src/collision/fcl/fcl_collision_plugin.cpp
  src/collision/allowed_collision_matrix.cpp
  src/collision/fcl/geometry_cache.cpp
  src/forward/frame_definitions.cpp
  src/forward/utilities/compute_joint_tree.cpp
  src/forward/utilities/compute_frame_tree.cpp
  src/forward/utilities/eigen_forward_kinematics_plugin.cpp
  src/forward/utilities/analysis_tree.cpp

  src/plugin_loader.cpp
  include/arm_kinematics/collision/collision_manager.cpp
  include/arm_kinematics/collision/collision_manager.hpp
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
set(TEST_KINEMATICS_SOURCE_FILES
  test/collision/fcl/test_allowed_collision_matrix.cpp
  test/forward/eigen/test_eigen_fk_mapper.cpp
  include/arm_kinematics/utilities/collider_definitions.cpp
  include/arm_kinematics/utilities/collider_definitions.hpp
)
if(NIX_DO_CHECK)
  target_link_libraries(test_kinematics
    ${PROJECT_NAME}
    fcl
  )
  target_include_directories(test_kinematics PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
  set_tests_properties(test_kinematics
    PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}"
  )
endif()

# -- Install

# Install header files
install(
  DIRECTORY include/
  DESTINATION include
)

# Install config files
#install(
#  DIRECTORY config/
#  DESTINATION share/${PROJECT_NAME}/config
#)

install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

ament_export_libraries(
  ${PROJECT_NAME}
)


# Manual test cases
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
# At the bottom of your CMakeLists.txt
add_executable(test_manual
  ${TEST_KINEMATICS_SOURCE_FILES}
)
target_link_libraries(test_manual
  ${PROJECT_NAME}
  GTest::gtest_main
  Threads::Threads
  fcl
)
target_include_directories(test_manual PUBLIC
  include
)
install(TARGETS
  test_manual
  DESTINATION lib/${PROJECT_NAME}
)


ament_export_targets(
  #  ${PROJECT_NAME}Targets
  export_${PROJECT_NAME}
  HAS_LIBRARY_TARGET
)

ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
