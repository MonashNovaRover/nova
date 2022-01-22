# Find and include arm-specific dependencies
find_package(sensor_msgs REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Orocos_kdl REQUIRED)

# Additional include for KDL and Eigen
include_directories(
  ${EIGEN3_INCLUDE_DIRS}
  include
)



################################################
# Add non-executable targets:
################################################

# Arm Core
add_library(arm_core arm_core/arm_core.cpp)
target_include_directories(arm_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/arm_core)
ament_target_dependencies(arm_core sensor_msgs)

# Arm Model
add_library(arm_model kinematics/arm_model.cpp)
target_link_libraries(arm_model orocos-kdl arm_core)
target_include_directories(arm_model INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/kinematics)
ament_target_dependencies(arm_model sensor_msgs)



################################################
# Add executables:
################################################

# Arm Publisher script
add_executable(arm_inputs arm_inputs.cpp)
target_link_libraries(arm_inputs arm_core print)
target_include_directories(arm_inputs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
ament_target_dependencies(arm_inputs rclcpp core)

# Arm Driver script
add_executable(arm_driver
  arm_driver.cpp
  joint.cpp
)
target_link_libraries(arm_driver arm_core cmd print)
target_include_directories(arm_inputs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
ament_target_dependencies(arm_driver rclcpp core)

# Arm Kinematics script
add_executable(arm_kinematics kinematics/arm_kinematics.cpp)
target_link_libraries(arm_kinematics orocos-kdl arm_core arm_model)
target_include_directories(arm_kinematics INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/kinematics)
ament_target_dependencies(arm_kinematics rclcpp sensor_msgs)

# Arm Simulator script
add_executable(arm_simulator simulator/arm_simulator.cpp)
target_link_libraries(arm_simulator arm_core)
target_include_directories(arm_simulator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/simulator)
ament_target_dependencies(arm_simulator rclcpp sensor_msgs)



################################################
# Add installations:
################################################

# Add installation for executable targets
install(TARGETS
  arm_inputs
  arm_driver
  arm_kinematics
  arm_simulator
  DESTINATION lib/${PROJECT_NAME}
)