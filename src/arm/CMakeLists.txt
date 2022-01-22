# Find and include arm-specific dependencies
find_package(sensor_msgs REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Orocos_kdl REQUIRED)

# Additional includes for arm nodes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${EIGEN3_INCLUDE_DIRS}
)

# Add non-executable targets

# Arm Core
add_library(arm_core armcore/arm_core.cpp)
target_include_directories(arm_core INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/armcore"
)
ament_target_dependencies(arm_core sensor_msgs)


# Arm Model
add_library(arm_model kinematics/arm_model.cpp)
target_link_libraries(arm_model orocos-kdl arm_core)
ament_target_dependencies(arm_model sensor_msgs)


# Add arm executables with relevant dependencies

# Arm Publisher script
add_executable(arm_input arm_publisher.cpp)
target_link_libraries(arm_input arm_core)
ament_target_dependencies(arm_input rclcpp core)

# Arm Driver script
add_executable(arm_driver arm_driver.cpp joint.cpp ${PROJECT_SOURCE_DIR}/src/cmd/cmd.cpp ${PROJECT_SOURCE_DIR}/include/socketcan/socketcan_cpp.cpp)
target_link_libraries(arm_driver arm_core)
ament_target_dependencies(arm_driver rclcpp core)

# Arm Kinematics script
add_executable(arm_kinematics kinematics/arm_kinematics.cpp)
target_link_libraries(arm_kinematics orocos-kdl arm_core arm_model)
ament_target_dependencies(arm_kinematics rclcpp sensor_msgs)

# Arm Simulator script
add_executable(arm_simulator simulator/arm_simulator.cpp)
target_link_libraries(arm_simulator arm_core)
ament_target_dependencies(arm_simulator rclcpp sensor_msgs)

# Add all the installation targets
install(TARGETS
  arm_core
  arm_input
  arm_driver
  arm_kinematics
  arm_simulator
  DESTINATION lib/${PROJECT_NAME}
)