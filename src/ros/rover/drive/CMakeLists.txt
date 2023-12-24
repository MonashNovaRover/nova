cmake_minimum_required(VERSION 3.8)
project(drive)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(include)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(drive_msgs REQUIRED)
find_package(blcmd_interfaces REQUIRED)
find_package(core REQUIRED)

#find libcanmd
find_package(libcanmd REQUIRED)

add_executable(driver src/driver.cpp)
target_link_libraries(driver PUBLIC libcanmd)
target_include_directories(driver PUBLIC 
      ${libcanmd_INCLUDE_DIRS})
ament_target_dependencies(driver PUBLIC rclcpp drive_msgs blcmd_interfaces)

add_executable(drive_inputs src/drive_inputs.cpp)
ament_target_dependencies(drive_inputs rclcpp core drive_msgs)

install(TARGETS
        driver
        drive_inputs
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

ament_package()