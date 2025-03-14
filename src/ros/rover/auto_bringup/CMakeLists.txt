cmake_minimum_required(VERSION 3.5)
project(auto_bringup)

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

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclpy REQUIRED)  
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

# Install Python scripts to make them available with `ros2 run auto_bringup <script_name>`
install(
  PROGRAMS
    rviz/save_waypoints.py
    rviz/load_waypoints.py
  DESTINATION lib/${PROJECT_NAME}
)

# Add all the installation directories
install(DIRECTORY
  launch
  params
  resources
  rviz
  maps
  DESTINATION share/${PROJECT_NAME}
)

ament_package()


