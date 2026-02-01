cmake_minimum_required(VERSION 3.5)
project(science)

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


# Find and include all dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)

find_package(science_interfaces REQUIRED)
find_package(input_interfaces REQUIRED)

# Include the include directory
include_directories(
  include
    ${catkin_INCLUDE_DIRS}
    "${PROJECT_SOURCE_DIR}/include"
)

# C++ executables
#add_executable(#name #source.cpp #dependency.cpp)
#ament_target_dependencies(#name rclcpp core)

# Add all the installation directories
install(DIRECTORY
  #launch
  DESTINATION share/${PROJECT_NAME}
)

# Install Python modules
ament_python_install_package(${PROJECT_NAME})

# Add all Python executables to this list
install(PROGRAMS
        science/arc/chute.py
        science/arc/tool_rotator.py
        science/arc/arc_spinny_part.py
        science/arc/arc_sweeper_servo.py
        science/arc/kiln.py
        science/arc/kiln_old.py
        science/arc/peltier.py
        science/urc/hydraprobe_trigger_node.py
        science/urc/urc_bme_sensor.py
        science/urc/urc_cache.py
        science/urc/urc_carousel.py
        science/urc/urc_heater.py
        science/urc/urc_hydraprobe.py
        science/urc/urc_hydraprobe_control.py
        science/urc/urc_mixers.py
        science/urc/urc_pumps.py
        science/urc/urc_raman_mech_server.py
        science/urc/urc_raman_spec_server.py
        science/urc/urc_theta_360_cam.py
        science/urc/urc_uv_vis_leds.py
        science/urc/urc_uv_vis_spec.py
        science/analysis_arm.py
        science/analysis_arm_stepper.py
        science/auger.py
        science/cbeam.py
        science/auger_old.py
        science/nir_probe_publisher.py
        science/scimbal_cam_old.py
        science/scimbal_cam.py
        science/control_test.py
        DESTINATION lib/${PROJECT_NAME}
)


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
