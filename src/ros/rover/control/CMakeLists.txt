cmake_minimum_required(VERSION 3.5)
project(control)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find and include ROS2 dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
# Find and include message types dependencies
find_package(core REQUIRED)
################################################
# Add non-executable targets:
################################################

# Libraries:
# Gamepad
if (NOT TARGET gamepad)
  add_library(gamepad include/gamepad/gamepad.c)
  target_link_libraries(gamepad udev)
endif()

# CAN (socketcan)
if (NOT TARGET socketcan)
  add_library(socketcan include/socketcan/socketcan_cpp.cpp)
endif()

# Print functions
if (NOT TARGET print)
  add_library(print include/print/print.cpp)
endif()
# Other targets:
# CMDs
add_library(cmd src/cmd/cmd.cpp)
target_link_libraries(cmd socketcan print)

################################################
# Add executables:
################################################

# Inputs script
add_executable(inputs
  src/inputs/inputs_publisher.cpp
  src/inputs/joystick.cpp 
  src/inputs/joystick_gamepad.cpp 
  src/inputs/joystick_thrustmaster.cpp
)
target_link_libraries(inputs gamepad print keyboard)
ament_target_dependencies(inputs rclcpp core SDL2)


# PID Tuner script
add_executable(pid_tuner src/cmd/pid_tuner.cpp)
target_link_libraries(pid_tuner cmd print)
ament_target_dependencies(pid_tuner rclcpp core)

################################################
# Add Installations
################################################

# Add all the installation targets
install(TARGETS
        inputs
        drive_inputs
        pid_tuner
        DESTINATION lib/${PROJECT_NAME}
        )

install(DIRECTORY
        rviz
        DESTINATION share/${PROJECT_NAME}
      )

# Add all Python executables to this list
install(PROGRAMS
        control/cmd_plotter.py
        control/rover_state_publisher.py
        control/blcmd_status_monitor.py
        control/tile_placer.py
        control/scraper.py
        control/gimbal_cam.py
        control/drive_sim.py
        control/drive_input_stamped_publisher.py
        control/drive_input_publisher.py
        control/auger.py
        control/analysis_platform.py
        DESTINATION lib/${PROJECT_NAME}
        )

# Install Python modules
ament_python_install_package(${PROJECT_NAME})

################################################
# Add Subdirectories
################################################

# Arm
add_subdirectory(src/arm)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
