cmake_minimum_required(VERSION 3.5)
project(arm_kinematics_benchmark LANGUAGES CXX)

# Prefer the active toolchain glibc at runtime. Some ROS/Nix dependency
# closures add an older glibc path into target RUNPATH, which can leave the
# binary requesting one loader while resolving libc from another store path.
execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libc.so.6
  OUTPUT_VARIABLE ARM_KINEMATICS_BENCHMARK_TOOLCHAIN_LIBC
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(IS_ABSOLUTE "${ARM_KINEMATICS_BENCHMARK_TOOLCHAIN_LIBC}")
  get_filename_component(
    ARM_KINEMATICS_BENCHMARK_TOOLCHAIN_GLIBC_DIR
    "${ARM_KINEMATICS_BENCHMARK_TOOLCHAIN_LIBC}"
    DIRECTORY
  )
  list(PREPEND CMAKE_BUILD_RPATH "${ARM_KINEMATICS_BENCHMARK_TOOLCHAIN_GLIBC_DIR}")
  list(PREPEND CMAKE_INSTALL_RPATH "${ARM_KINEMATICS_BENCHMARK_TOOLCHAIN_GLIBC_DIR}")
endif()

set(THIS_PACKAGE_INCLUDE_DEPENDS
  arm_kinematics
  fcl
  rclcpp
)

find_package(ament_cmake REQUIRED)
find_package(benchmark REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

# -- Benchmarks

add_executable(benchmark_joint_map
  joint_map_builder_benchmark.cpp
)
target_compile_features(benchmark_joint_map PUBLIC cxx_std_17)
ament_target_dependencies(benchmark_joint_map arm_kinematics)
target_link_libraries(benchmark_joint_map
  benchmark::benchmark
  benchmark::benchmark_main
  Threads::Threads
)
install(TARGETS benchmark_joint_map
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(benchmark_twistmapper_collision
  twistmapper_benchmark_support.cpp
  twistmapper_collision_benchmark.cpp
)
target_compile_features(benchmark_twistmapper_collision PUBLIC cxx_std_17)
ament_target_dependencies(benchmark_twistmapper_collision arm_kinematics rclcpp)
target_include_directories(benchmark_twistmapper_collision PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(benchmark_twistmapper_collision
  benchmark::benchmark
  benchmark::benchmark_main
  Threads::Threads
  fcl
)
target_compile_definitions(benchmark_twistmapper_collision PRIVATE
  ARM_KINEMATICS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  ARM_KINEMATICS_INSTALL_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/benchmark"
)
install(TARGETS benchmark_twistmapper_collision
  DESTINATION lib/${PROJECT_NAME}
)
add_executable(benchmark_nova_twistmapper
  twistmapper_benchmark_support.cpp
  nova_twistmapper_benchmark.cpp
)
target_compile_features(benchmark_nova_twistmapper PUBLIC cxx_std_17)
ament_target_dependencies(benchmark_nova_twistmapper arm_kinematics rclcpp)
target_include_directories(benchmark_nova_twistmapper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(benchmark_nova_twistmapper
  benchmark::benchmark
  benchmark::benchmark_main
  Threads::Threads
  fcl
)
target_compile_definitions(benchmark_nova_twistmapper PRIVATE
  ARM_KINEMATICS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  ARM_KINEMATICS_INSTALL_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/benchmark"
)
install(TARGETS benchmark_nova_twistmapper
  DESTINATION lib/${PROJECT_NAME}
)

add_executable(test_twistmapper_collision
  twistmapper_benchmark_support.cpp
  twistmapper_collision_test.cpp
)
target_compile_features(test_twistmapper_collision PUBLIC cxx_std_17)
ament_target_dependencies(test_twistmapper_collision arm_kinematics rclcpp)
target_include_directories(test_twistmapper_collision PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_twistmapper_collision
  GTest::gtest_main
  Threads::Threads
  fcl
)
target_compile_definitions(test_twistmapper_collision PRIVATE
  ARM_KINEMATICS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
  ARM_KINEMATICS_INSTALL_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/benchmark"
)
install(TARGETS test_twistmapper_collision
  DESTINATION lib/${PROJECT_NAME}
)

install(
  FILES data/taipan_twistmapper_benchmark.urdf
  DESTINATION share/${PROJECT_NAME}/benchmark
)

ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
