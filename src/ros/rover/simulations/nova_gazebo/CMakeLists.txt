cmake_minimum_required(VERSION 3.5)
project(nova_gazebo)

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
find_program(XACRO_EXECUTABLE xacro REQUIRED)
# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

file(GLOB WORLD_TEMPLATE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} worlds/*.sdf)

set(GENERATED_WORLDS_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_worlds)
set(GENERATED_WORLD_FILES)

foreach(WORLD_TEMPLATE ${WORLD_TEMPLATE_FILES})
  get_filename_component(WORLD_NAME ${WORLD_TEMPLATE} NAME)
  set(GENERATED_WORLD ${GENERATED_WORLDS_DIR}/${WORLD_NAME})

  add_custom_command(
    OUTPUT ${GENERATED_WORLD}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_WORLDS_DIR}
    COMMAND ${XACRO_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${WORLD_TEMPLATE} -o ${GENERATED_WORLD}
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/${WORLD_TEMPLATE}
      ${CMAKE_CURRENT_SOURCE_DIR}/worlds/includes/world_base.xacro
    VERBATIM
  )

  list(APPEND GENERATED_WORLD_FILES ${GENERATED_WORLD})
endforeach()

add_custom_target(generate_worlds ALL DEPENDS ${GENERATED_WORLD_FILES})

install(DIRECTORY
  models
  DESTINATION share/${PROJECT_NAME}
)

install(FILES ${GENERATED_WORLD_FILES}
  DESTINATION share/${PROJECT_NAME}/worlds
)

# Run build testing package nodes
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
