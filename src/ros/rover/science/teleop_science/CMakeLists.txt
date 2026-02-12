cmake_minimum_required(VERSION 3.8)
project(teleop_science)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)

# Add all the installation directories
install(DIRECTORY
  launch
  params
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
