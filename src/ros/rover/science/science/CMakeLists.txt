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
        science/arc/kiln.py
        science/arc/kiln_door.py
        science/arc/sweeper.py
        science/arc/water_pump.py
        science/arc/diaphragm_pump.py
        science/urc/auger_hall_effect.py
        science/urc/bme_sensor.py
        science/urc/cache.py
        science/urc/carousel.py
        science/urc/heater.py
        science/urc/hydraprobe.py
        science/urc/litmus_dipper.py
        science/urc/potentiostat.py
        science/urc/pumps.py
        science/urc/spec_leds.py
        science/urc/urc_mixers.py
        science/urc/urc_raman_mech_server.py
        science/urc/urc_raman_spec_server.py
        science/urc/urc_theta_360_cam.py
        science/urc/urc_uv_vis_spec.py
        science/analysis_arm_stepper.py
        science/auger.py
        science/cbeam.py
        science/nir_probe.py
        science/power_cycle.py
        science/scimbal_cam.py
        science/control_test.py
        science/time_of_flight.py
        DESTINATION lib/${PROJECT_NAME}
)


if(BUILD_TESTING)

  # Run a specific Python test file
  ament_add_pytest_test(test_kiln_temp tests/test_kiln_temp.py)

  # Scan an entire directory for files matching test_*.py
  # ament_add_pytest_test(test_suite test/)
endif()

ament_package()
