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

# Find and include all dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(OpenCV REQUIRED)
find_package(realsense2 REQUIRED)
find_package(core REQUIRED)


pybind11_add_module(a_star src/a_star.cpp)
pybind11_add_module(height_mapper src/height_mapper.cpp)
pybind11_add_module(plane_fitter src/plane_generator.cpp)
pybind11_add_module(rs2_ros2 src/pc_converter.cpp)


###HACK : ament_target_dependencies cannot declare its deps as private, which is needed for pybind11.. so copypaste
function(ament_target_dependencies_private target)
  if(NOT TARGET ${target})
    message(FATAL_ERROR "ament_target_dependencies() the first argument must be a valid target name")
  endif()
  if(${ARGC} GREATER 0)
    set(definitions "")
    set(include_dirs "")
    set(libraries "")
    set(link_flags "")
    foreach(package_name ${ARGN})
      if(NOT ${${package_name}_FOUND})
        message(FATAL_ERROR "ament_target_dependencies() the passed package name '${package_name}' was not found before")
      endif()
      list_append_unique(definitions ${${package_name}_DEFINITIONS})
      list_append_unique(include_dirs ${${package_name}_INCLUDE_DIRS})
      list(APPEND libraries ${${package_name}_LIBRARIES})
      list_append_unique(link_flags ${${package_name}_LINK_FLAGS})
    endforeach()
    target_compile_definitions(${target}
      PUBLIC ${definitions})
    ament_include_directories_order(ordered_include_dirs ${include_dirs})
    target_include_directories(${target}
      PUBLIC ${ordered_include_dirs})
    ament_libraries_deduplicate(unique_libraries ${libraries})
    target_link_libraries(${target} PRIVATE
      ${unique_libraries})
    foreach(link_flag IN LISTS link_flags)
      set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " ${link_flag} ")
    endforeach()
  endif()
endfunction()

# Include the include directory
include_directories(
  include
    ${catkin_INCLUDE_DIRS}
    "${PROJECT_SOURCE_DIR}/include"
    "${OpenCV_INCLUDE_DIRS}"
)

target_link_libraries( height_mapper PRIVATE ${OpenCV_LIBS} )
target_link_libraries( plane_fitter PRIVATE ${OpenCV_LIBS} )
target_link_libraries( rs2_ros2 PRIVATE ${realsense2_LIBRARY})
ament_target_dependencies_private(
    rs2_ros2
    "sensor_msgs"
    "std_msgs"
    realsense2
    pybind11::module
    )


install(TARGETS a_star DESTINATION ~/nova_ws/install/autonomous/lib/python3.8/site-packages)
install(TARGETS height_mapper DESTINATION ~/nova_ws/install/autonomous/lib/python3.8/site-packages)
install(TARGETS plane_fitter DESTINATION ~/nova_ws/install/autonomous/lib/python3.8/site-packages)
install(TARGETS rs2_ros2 DESTINATION ~/nova_ws/install/autonomous/lib/python3.8/site-packages)


# Add all the installation directories
install(DIRECTORY
  rviz
  DESTINATION share/${PROJECT_NAME}
)

# Install Python modules
ament_python_install_package(${PROJECT_NAME})

# Add all Python executables to this list
install(PROGRAMS
  autonomous/main.py
  autonomous/cameras/tracking_camera.py
  autonomous/cameras/depth_camera.py
  autonomous/localisation/pose_converter_ARC.py
  autonomous/localisation/wheel_odometry.py
  autonomous/controller/GRUC.py
  autonomous/controller/GRUP.py
  autonomous/controller/goal_manager.py
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

ament_export_libraries(
    rs2_ros2
    )

ament_package()
