cmake_minimum_required(VERSION 3.8)
project(cameras)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(THIS_PACKAGE_INCLUDE_DEPENDS
  rclcpp
  std_msgs
  std_srvs
  camera_msgs
)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(generate_parameter_library REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)
include_directories(${SYSTEMD_INCLUDE_DIRS})

pkg_check_modules(GSTREAMERMM REQUIRED gstreamermm-1.0)
include_directories(${GSTREAMERMM_INCLUDE_DIRS})
link_directories(${GSTREAMERMM_LIB_DIRS})


include_directories(
  include
)

# Generate parameter libraries
generate_parameter_library(
  directory_parameters
  src/directory_parameters.yaml
)

add_executable(camera_directory_service src/camera_directory_service.cpp src/camera_globals.cpp)
target_link_libraries(camera_directory_service ${SYSTEMD_LIBRARIES} directory_parameters)
ament_target_dependencies(camera_directory_service ${THIS_PACKAGE_INCLUDE_DEPENDS})

add_executable(camera_streamer_service src/camera_streamer_service.cpp src/camera_globals.cpp src/camera_pipelines.cpp)
target_link_libraries(camera_streamer_service ${GSTREAMERMM_LIBRARIES})
ament_target_dependencies(camera_streamer_service ${THIS_PACKAGE_INCLUDE_DEPENDS})

# Install executables
install(
  TARGETS 
    camera_directory_service 
    directory_parameters 
  DESTINATION lib/${PROJECT_NAME}
)
install(
  TARGETS 
    camera_streamer_service 
  DESTINATION lib/${PROJECT_NAME}
)
# Install param files
install(
  DIRECTORY params
  DESTINATION share/${PROJECT_NAME}
)

# Install header files
install(
  DIRECTORY include
  DESTINATION include/${PROJECT_NAME}
)

# Install launch files
install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)


if(BUILD_TESTING)
endif()

ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
