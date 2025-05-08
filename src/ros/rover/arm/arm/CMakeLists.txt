cmake_minimum_required(VERSION 3.8)
project(arm)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python)
find_package(rclcpp REQUIRED)


# Find and include arm-specific dependencies
find_package(Eigen3 REQUIRED)
find_package(Orocos_kdl REQUIRED)

# Additional message type dependencies
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(std_srvs REQUIRED)

find_package(arm_interfaces REQUIRED)
find_package(input_interfaces REQUIRED)
find_package(nova_interfaces REQUIRED)

#For Keyboard Translate, Ideally we remove this as a dependency and only Inputs depends on SDL2
find_package(SDL2 REQUIRED)

find_package(libcanmd REQUIRED)

find_package(inputs REQUIRED)

include_directories(
  ${EIGEN3_INCLUDE_DIRS}
  include
  include/arm
)


################################################
# Add non-executable targets:
################################################

# Arm Messages
add_library(arm_messages src/arm_core/arm_messages.cpp)
target_include_directories(arm_messages INTERFACE arm_core)
ament_target_dependencies(arm_messages sensor_msgs)

# Arm Type Translation
add_library(arm_type_translation src/arm_core/arm_type_translation.cpp)
target_link_libraries(arm_type_translation orocos-kdl)
target_include_directories(arm_type_translation INTERFACE arm_core)
ament_target_dependencies(arm_type_translation sensor_msgs geometry_msgs)

# Arm Config Info Client
add_library(arm_config_info_client src/arm_core/arm_config_info_client.cpp)
target_include_directories(arm_config_info_client INTERFACE arm_core)
ament_target_dependencies(arm_config_info_client rclcpp arm_interfaces)

# Arm Model
add_library(arm_model src/model/arm_model.cpp)
target_link_libraries(arm_model orocos-kdl libcanmd)
target_include_directories(arm_model INTERFACE model ${libcanmd_INCLUDE_DIRS})

# Arm Kinematics
add_library(arm_kinematics
  src/kinematics/arm_kinematics.cpp
  src/kinematics/spm_kinematics.cpp
)
target_link_libraries(arm_kinematics orocos-kdl arm_messages arm_model)
target_include_directories(arm_kinematics INTERFACE kinematics)
ament_target_dependencies(arm_kinematics rclcpp arm_interfaces geometry_msgs sensor_msgs)

# PI Controller
add_library(pi_controller src/arm_control/pi_controller.cpp)
target_include_directories(pi_controller INTERFACE arm_control)

# Joystick Translate
add_library(joystick_translate src/arm_inputs/joystick_translate.cpp)
target_include_directories(joystick_translate PUBLIC arm_inputs)
ament_target_dependencies(joystick_translate input_interfaces arm_interfaces geometry_msgs sensor_msgs)

# Keyboard Translate
add_library(keyboard_translate src/arm_inputs/keyboard_translate.cpp)
target_include_directories(keyboard_translate PUBLIC arm_inputs)
ament_target_dependencies(keyboard_translate input_interfaces arm_interfaces geometry_msgs sensor_msgs SDL2 inputs)


################################################
# Add executables:
################################################

# Arm Inputs script
add_executable(arm_inputs src/arm_inputs/arm_inputs.cpp)
target_link_libraries(arm_inputs arm_messages arm_config_info_client keyboard_translate joystick_translate)
target_include_directories(arm_inputs INTERFACE arm_inputs)
ament_target_dependencies(arm_inputs rclcpp geometry_msgs sensor_msgs input_interfaces)

# Arm Twist Mapper script
add_executable(arm_twistmapper src/arm_control/arm_twistmapper.cpp)
target_link_libraries(arm_twistmapper orocos-kdl arm_messages arm_type_translation arm_model arm_kinematics)
target_include_directories(arm_twistmapper INTERFACE arm_control)
ament_target_dependencies(arm_twistmapper rclcpp arm_interfaces geometry_msgs std_srvs)

# Arm Control script
add_executable(arm_control src/arm_control/arm_control.cpp)
target_link_libraries(arm_control orocos-kdl arm_messages arm_type_translation arm_model arm_kinematics pi_controller)
target_include_directories(arm_control INTERFACE arm_control)
ament_target_dependencies(arm_control rclcpp arm_interfaces geometry_msgs sensor_msgs std_srvs)

# Arm Driver script
add_executable(arm_driver src/arm_driver/arm_driver.cpp)
target_link_libraries(arm_driver arm_messages arm_config_info_client arm_model libcanmd)
target_include_directories(arm_inputs INTERFACE arm_driver ${libcanmd_INCLUDE_DIRS})
ament_target_dependencies(arm_driver rclcpp arm_interfaces sensor_msgs)

# Arm rviz publisher
add_executable(arm_rviz_publisher src/arm_rviz_publisher/arm_rviz_publisher.cpp)
target_link_libraries(arm_rviz_publisher)
target_include_directories(arm_rviz_publisher INTERFACE arm_rviz_publisher)
ament_target_dependencies(arm_rviz_publisher rclcpp arm_interfaces sensor_msgs nav_msgs)

# Resolver Spoofer script
add_executable(resolver_spoofer src/resolver_spoofer/resolver_spoofer.cpp)
target_link_libraries(resolver_spoofer arm_messages arm_config_info_client)
target_include_directories(resolver_spoofer INTERFACE resolver_spoofer)
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
  arm_rviz_publisher
  resolver_spoofer
  DESTINATION lib/${PROJECT_NAME}
)

# Add all Python executables to this list
install(PROGRAMS
        arm/resolver_publisher.py
        arm/zero_resolver_client_terminal.py
        arm/zero_resolver_sector_client_terminal.py
        arm/hex_key.py
        arm/lazers.py
        arm/keyboard_localiser.py
        arm/typing_sequencer.py
        DESTINATION lib/${PROJECT_NAME}
        )

# Install Python modules
ament_python_install_package(${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
