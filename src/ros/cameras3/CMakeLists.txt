cmake_minimum_required(VERSION 3.8)
project(cameras3)

set(THIS_PACKAGE_INCLUDE_DEPENDS
  rclcpp
  std_msgs
)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
#find_package(generate_parameter_library REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)
include_directories(${SYSTEMD_INCLUDE_DIRS})

include_directories(
  include
)

add_executable(cam3_dir src/camera_directory_service.cpp)
target_link_libraries(cam3_dir ${SYSTEMD_LIBRARIES})
ament_target_dependencies(cam3_dir ${THIS_PACKAGE_INCLUDE_DEPENDS})

install(TARGETS cam3_dir DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
endif()

ament_package()
