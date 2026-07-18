cmake_minimum_required(VERSION 3.5)
project(lattice_primitive_generator)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)

ament_python_install_package(${PROJECT_NAME})

install(
  PROGRAMS
    lattice_primitive_generator/generate_motion_primitives.py
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY
    lattice_primitive_generator
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
