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
pybind11_add_module(a_star src/a_star.cpp)
pybind11_add_module(height_mapper src/height_mapper.cpp)
pybind11_add_module(plane_fitter src/plane_generator.cpp)
install(TARGETS a_star DESTINATION ~/nova_ws/install/autonomous/lib/python3.8/site-packages)
install(TARGETS height_mapper DESTINATION ~/nova_ws/install/autonomous/lib/python3.8/site-packages)
install(TARGETS plane_fitter DESTINATION ~/nova_ws/install/autonomous/lib/python3.8/site-packages)

# Find and include all dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(OpenCV REQUIRED)

# Include the core package
find_package(core REQUIRED)

# Include the include directory
include_directories(
  include
    ${catkin_INCLUDE_DIRS}
    "${PROJECT_SOURCE_DIR}/include"
    "${OpenCV_INCLUDE_DIRS}"
)

target_link_libraries( height_mapper PRIVATE ${OpenCV_LIBS} )
target_link_libraries( plane_fitter PRIVATE ${OpenCV_LIBS} )

#ament_target_dependencies(#name rclcpp core)

# Add all the installation directories
install(DIRECTORY
  #launch
  DESTINATION share/${PROJECT_NAME}
)

# Add all Python executables to this list
install(PROGRAMS
  autonomous/main.py
  autonomous/cameras/ar_tracker.py
  autonomous/cameras/depth_camera.py
  autonomous/cameras/tracking_camera.py
  autonomous/config/ros_config.py
  autonomous/config/runtime_params.py
  autonomous/controller/GRUC.py
  autonomous/controller/drive_controller.py
  autonomous/controller/ar_tag_manager.py
  autonomous/controller/spin_controller.py
  autonomous/controller/turning.py
  autonomous/mapping/flat_mapper.py
  autonomous/mapping/grid_2d.py
  autonomous/mapping/height_plane_mapper.py
  autonomous/mapping/mapper.py
  autonomous/mapping/python_height_mapper.py
  autonomous/mapping/python_plane_mapper.py
  autonomous/math_utils/controller_math.py
  autonomous/math_utils/transform.py
  autonomous/planning/path_planner.py
  autonomous/resources/rover.npy
  autonomous/resources/environment.npy
  autonomous/resources/rover.ply
  autonomous/vis/grid_pub.py
  autonomous/vis/path_vis.py
  autonomous/vis/pc_converter.py
  autonomous/vis/pc_pub.py
  autonomous/vis/rover_vis.py
  autonomous/main.py
  autonomous/update_goals.py

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
