# Find and include arm-specific dependencies
find_package(Eigen3 QUIET)
find_package(Orocos_kdl QUIET)

# Only continue if the packages were found
# In future, use this to set a #define in the C++ so can still run in joint space with no FK / IK
if(Eigen3_FOUND AND Orocos_kdl_FOUND)
  
  # Additional message type dependencies
  find_package(sensor_msgs REQUIRED)
  find_package(geometry_msgs REQUIRED)
  find_package(nav_msgs REQUIRED)

  # Additional includes for Eigen adn KDL
  include_directories(
    ${EIGEN3_INCLUDE_DIRS}
    include
  )


  ################################################
  # Add non-executable targets:
  ################################################

  # Arm Messages
  add_library(arm_messages arm_core/arm_messages.cpp)
  target_include_directories(arm_messages INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/arm_core)
  ament_target_dependencies(arm_messages sensor_msgs)

  # Arm Type Translation
  add_library(arm_type_translation arm_core/arm_type_translation.cpp)
  target_link_libraries(arm_type_translation orocos-kdl)
  target_include_directories(arm_type_translation INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/arm_core)
  ament_target_dependencies(arm_type_translation sensor_msgs geometry_msgs)

  # Arm Config Info Client
  add_library(arm_config_info_client arm_core/arm_config_info_client.cpp)
  target_link_libraries(arm_config_info_client print)
  target_include_directories(arm_config_info_client INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/arm_core)
  ament_target_dependencies(arm_config_info_client rclcpp core)

  # Arm Model
  add_library(arm_model model/arm_model.cpp)
  target_link_libraries(arm_model orocos-kdl)
  target_include_directories(arm_model INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/model)

  # Arm Kinematics
  add_library(arm_kinematics
    kinematics/arm_kinematics.cpp
    kinematics/spm_kinematics.cpp
  )
  target_link_libraries(arm_kinematics orocos-kdl arm_messages arm_model print)
  target_include_directories(arm_kinematics INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/kinematics)
  ament_target_dependencies(arm_kinematics rclcpp core geometry_msgs sensor_msgs)


  ################################################
  # Add executables:
  ################################################

  # Arm Inputs script
  add_executable(arm_inputs arm_inputs/arm_inputs.cpp)
  target_link_libraries(arm_inputs arm_messages arm_config_info_client print)
  target_include_directories(arm_inputs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/arm_inputs)
  ament_target_dependencies(arm_inputs rclcpp core geometry_msgs sensor_msgs)

  # Arm Twist Mapper script
  add_executable(arm_twistmapper arm_control/arm_twistmapper.cpp)
  target_link_libraries(arm_twistmapper orocos-kdl arm_messages arm_type_translation arm_model arm_kinematics print)
  target_include_directories(arm_twistmapper INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/arm_control)
  ament_target_dependencies(arm_twistmapper rclcpp core geometry_msgs)

  # Arm Control script
  add_executable(arm_control arm_control/arm_control.cpp)
  target_link_libraries(arm_control orocos-kdl arm_messages arm_type_translation arm_model arm_kinematics print)
  target_include_directories(arm_control INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/arm_control)
  ament_target_dependencies(arm_control rclcpp core geometry_msgs sensor_msgs)

  # Arm Driver script
  add_executable(arm_driver arm_driver/arm_driver.cpp)
  target_link_libraries(arm_driver arm_messages arm_config_info_client cmd print)
  target_include_directories(arm_inputs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/arm_driver)
  ament_target_dependencies(arm_driver rclcpp core sensor_msgs)

  # Resolver Spoofer script
  add_executable(resolver_spoofer resolver_spoofer/resolver_spoofer.cpp)
  target_link_libraries(resolver_spoofer arm_messages arm_config_info_client print)
  target_include_directories(resolver_spoofer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/resolver_spoofer)
  ament_target_dependencies(resolver_spoofer rclcpp sensor_msgs)


  ################################################
  # Add installations:
  ################################################

  # Add installation for executable targets
  install(TARGETS
    arm_inputs
    arm_twistmapper
    arm_control
    arm_driver
    resolver_spoofer
    DESTINATION lib/${PROJECT_NAME}
  )

endif()
