cmake_minimum_required(VERSION 3.5)
project(qcmd_hardware)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)

# find jcan
find_package(PkgConfig)
pkg_check_modules(JCAN REQUIRED jcan)

add_library(
  qcmd_hardware
  SHARED
  src/qcmd_hardware.cpp
)
target_include_directories(
  qcmd_hardware
  PUBLIC
  include
  ${JCAN_INCLUDE_DIRS}
)

target_link_libraries(
  qcmd_hardware
  ${JCAN_LIBRARIES}
)

ament_target_dependencies(
  qcmd_hardware
  hardware_interface
  rclcpp
  rclcpp_lifecycle
)
# prevent pluginlib from using boost
target_compile_definitions(qcmd_hardware PUBLIC "PLUGINLIB__DISABLE_BOOST_FUNCTIONS")

pluginlib_export_plugin_description_file(
  hardware_interface qcmd_hardware.xml)

install(
  TARGETS
  qcmd_hardware
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

install(
  DIRECTORY include/
  DESTINATION include
)

#if(BUILD_TESTING)
#  find_package(ament_lint_auto REQUIRED)
#  # the following line skips the linter which checks for copyrights
#  # uncomment the line when a copyright and license is not present in all source files
#  #set(ament_cmake_copyright_FOUND TRUE)
#  # the following line skips cpplint (only works in a git repo)
#  # uncomment the line when this package is not in a git repo
#  #set(ament_cmake_cpplint_FOUND TRUE)
#  ament_lint_auto_find_test_dependencies()
##  find_package(ament_cmake_gmock REQUIRED)
#  find_package(ros2_control_test_assets REQUIRED)
#
##  ament_add_gmock(test_qcmd_hardware test/test_qcmd_hardware.cpp)
#  target_include_directories(test_qcmd_hardware PRIVATE include)
#  ament_target_dependencies(
#    test_qcmd_hardware
#    hardware_interface
#    pluginlib
#    ros2_control_test_assets
#  )
#endif()

ament_export_include_directories(
  include
)
ament_export_libraries(
  qcmd_hardware
)
ament_export_dependencies(
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
)

ament_package()
