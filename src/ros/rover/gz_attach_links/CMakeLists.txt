cmake_minimum_required(VERSION 3.10.2)
project(gz_attach_links)

find_package(ament_cmake REQUIRED)
find_package(gz_plugin_vendor REQUIRED)
find_package(gz-plugin2 REQUIRED)
find_package(gz_sim_vendor REQUIRED)
find_package(gz-sim8 REQUIRED)
find_package(gz_common_vendor REQUIRED)
find_package(gz-common5 REQUIRED)

ament_environment_hooks("hooks/resource_paths.dsv.in")

add_definitions(-DGZ_NS=gz)

add_library(attach-links SHARED src/attach_links/AttachLinks.cc)

target_include_directories(attach-links PRIVATE
  gz/plugin2
  gz/sim8
  gz/common5
)

target_link_libraries(attach-links
  gz-plugin2::core
  gz-sim8::core
  gz-common5::core
)

install(DIRECTORY 
  launch 
  models 
  DESTINATION share/${PROJECT_NAME}
)

install(TARGETS 
  attach-links 
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

ament_package()
