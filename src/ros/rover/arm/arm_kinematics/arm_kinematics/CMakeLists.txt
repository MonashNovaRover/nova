cmake_minimum_required(VERSION 3.5)
project(arm_kinematics LANGUAGES CXX)

# Prefer the active toolchain glibc at runtime. Some ROS/Nix dependency
# closures add an older glibc path into target RUNPATH, which can leave the
# binary requesting one loader while resolving libc from another store path.
execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libc.so.6
  OUTPUT_VARIABLE ARM_KINEMATICS_TOOLCHAIN_LIBC
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(IS_ABSOLUTE "${ARM_KINEMATICS_TOOLCHAIN_LIBC}")
  get_filename_component(
    ARM_KINEMATICS_TOOLCHAIN_GLIBC_DIR
    "${ARM_KINEMATICS_TOOLCHAIN_LIBC}"
    DIRECTORY
  )
  list(PREPEND CMAKE_BUILD_RPATH "${ARM_KINEMATICS_TOOLCHAIN_GLIBC_DIR}")
  list(PREPEND CMAKE_INSTALL_RPATH "${ARM_KINEMATICS_TOOLCHAIN_GLIBC_DIR}")
endif()

set(THIS_PACKAGE_INCLUDE_DEPENDS
  geometry_msgs
  pluginlib
  rclcpp
  rcpputils
  hardware_interface
  tf2
  tf2_msgs
  tf2_geometry_msgs
  Eigen3
  tf2_eigen
  transmission_interface
  fcl
  tinyxml2
)

find_package(ament_cmake REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

# pluginlib_export_plugin_description_file(arm_kinematics plugins.xml)

include_directories(
  include
)

# -- Libraries

# Add plugin implementations here!
add_library(${PROJECT_NAME} SHARED
  src/utilities/utilities.cpp
  src/joint_map/joint_map.cpp
  src/joint_map/joint_map_builder.cpp
  src/joint_map/affine_joint_map.cpp
  src/joint_map/composite_joint_map.cpp
  src/joint_map/default_joint_map_builder.cpp
  src/joint_map/constant_supplemented_joint_map.cpp
  src/joint_map/missing_input_resolution.cpp
  src/joint_map/transmission_analysis.cpp
  src/joint_map/transmission_analysis_import.cpp
  src/joint_map/transmission_analysis_view.cpp
  src/joint_map/ros2_control_transmission_plugin_loader.cpp
  src/joint_map/transmission_joint_map.cpp
  src/joint_map/transmission_reachability.cpp
  src/joint_map/joint_map_blueprint.cpp
  src/joint_map/materialize_joint_map.cpp
  src/common/kinematics_params.cpp
  src/common/kinematics_base.cpp
  src/forward/forward_kinematics_plugin.cpp
  src/inverse/inverse_kinematics_plugin.cpp
  src/collision/discrete_collision_plugin.cpp
  src/collision/allowed_collision_matrix.cpp
  src/collision/collision_utilities.cpp
  src/forward/frame_definitions.cpp
  src/forward/utilities/compute_joint_tree.cpp
  src/forward/utilities/compute_frame_tree.cpp
  src/forward/utilities/analysis_tree.cpp
  src/plugin_loader.cpp
  src/collision/collision_manager.cpp
  src/collision/collider_definitions.cpp
  src/common/robot_model.cpp
  src/kinematics.cpp
  src/ros2_control/interface_lookup_error.cpp
  src/ros2_control/interface_refs.cpp
  src/ros2_control/joint_interface_bundle.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)  # Require C++17
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

ament_target_dependencies(${PROJECT_NAME} ${THIS_PACKAGE_INCLUDE_DEPENDS})

# Stuff for plugins
add_library(${PROJECT_NAME}_plugins SHARED
  src/plugins/collision/fcl/fcl_collision_plugin.cpp
  src/plugins/collision/fcl/geometry_cache.cpp
  src/plugins/forward/default_forward_kinematics_plugin.cpp
  src/plugins/inverse/banksia_ik_plugin.cpp
)
target_compile_features(${PROJECT_NAME}_plugins PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME}_plugins PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(${PROJECT_NAME}_plugins ${THIS_PACKAGE_INCLUDE_DEPENDS})
target_link_libraries(${PROJECT_NAME}_plugins ${PROJECT_NAME} fcl)
pluginlib_export_plugin_description_file(${PROJECT_NAME} plugins.xml)
install(FILES plugins.xml
  DESTINATION share/${PROJECT_NAME}
)

# Test cases
set(TEST_KINEMATICS_SOURCE_FILES
  test/collision/fcl/test_allowed_collision_matrix.cpp
  test/collision/fcl/test_fcl_collision_plugin.cpp
  test/collision/test_collision_path.cpp
  test/collision/test_collision_utilities.cpp
  test/common/test_robot_model_urdf.cpp
  test/forward/eigen/test_eigen_make_tree.cpp
  test/joint_map/test_default_joint_map_builder.cpp
  test/joint_map/test_joint_map_blueprint.cpp
  test/joint_map/test_materialize_joint_map.cpp
  test/joint_map/test_transmission_analysis.cpp
  test/joint_map/test_transmission_analysis_urdf_import.cpp
  test/joint_map/test_transmission_reachability.cpp
  test/ros2_control/test_interface_names.cpp
  test/ros2_control/test_interface_refs.cpp
  test/ros2_control/test_joint_interface_bundle.cpp
  test/utilities/test_interface_id.cpp
  test/utilities/test_rigid_body_inertia.cpp
  test/utilities/test_twist_wrench.cpp
  test/main.cpp
)
#if(NIX_DO_CHECK)
#  target_link_libraries(test_manual
#    ${PROJECT_NAME}
#    fcl
#  )
#  target_include_directories(test_kinematics PUBLIC
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#  )
#  set_tests_properties(test_kinematics
#    PROPERTIES
#    ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}:$ENV{LD_LIBRARY_PATH}"
#  )
#endif()

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

install(TARGETS ${PROJECT_NAME}_plugins
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
  ${PROJECT_NAME}_plugins
  GTest::gtest_main
  Threads::Threads
  fcl
)
target_include_directories(test_manual PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
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
