cmake_minimum_required(VERSION 3.8)
project(electronics)

# === Environment ===

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

# === Dependencies ===

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

# Include the nova_interfaces package
find_package(nova_interfaces REQUIRED)

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
  electronics/rfid_client_terminal.py
  electronics/rfid_service.py
  electronics/gimbal_service.py
  electronics/radio_monitor.py
  electronics/tests/test_EMC.py
  electronics/tests/test_GPS.py
  electronics/tests/test_radio.py
  electronics/tests/test_wheels.py
  electronics/wheel_publisher.py
  electronics/LED_transmitter.py
  electronics/dgps/base_gps_sub.py
  electronics/dgps/gps_publisher.py
  electronics/led_strip.py
  electronics/dgps/gps_base.py
  electronics/dgps/gps_data_collector.py
  electronics/dgps/gps_data_plotter.py
  electronics/dgps/gps_rover.py
  electronics/dgps/gps_test_localisation.py
  electronics/led_strip.py
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
