cmake_minimum_required(VERSION 3.5)
project(autonomous)

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

# add pybind stuff so we can build pybind wrappers
add_subdirectory(pybind11)
pybind11_add_module(path_planner src/path_planner.cpp)
install(TARGETS path_planner DESTINATION ~/nova_ws/install/autonomous/lib/python3.6/site-packages)

# Find and include all dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)

# Include the core package
find_package(core REQUIRED)

# Include the include directory
include_directories(
  include
    ${catkin_INCLUDE_DIRS}
    "${PROJECT_SOURCE_DIR}/include"
)


#ament_target_dependencies(#name rclcpp core)

# Add all the installation directories
install(DIRECTORY
  #launch
  DESTINATION share/${PROJECT_NAME}
)

# Add all Python executables to this list
install(PROGRAMS
  #autonomous/python.py
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
