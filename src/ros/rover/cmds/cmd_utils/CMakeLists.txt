cmake_minimum_required(VERSION 3.8)
project(cmd_utils)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(cmd_interfaces REQUIRED)
find_package(libcanmd REQUIRED)

include_directories(
  include/cmd_utils
)

add_executable(pid_tuner src/pid_tuner.cpp)
target_link_libraries(pid_tuner libcanmd)
target_include_directories(pid_tuner INTERFACE ${libcanmd_INCLUDE_DIRS})
ament_target_dependencies(pid_tuner rclcpp cmd_interfaces)

install(TARGETS
        pid_tuner
        DESTINATION lib/${PROJECT_NAME}
        )

install(PROGRAMS
        cmd_utils/cmd_plotter.py
        cmd_utils/CMD_publisher.py
        cmd_utils/CMD_service.py
        DESTINATION lib/${PROJECT_NAME}
        )


if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_python_install_package(${PROJECT_NAME})

ament_package()
