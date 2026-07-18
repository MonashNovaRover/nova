{ lib
, buildRosPackage
, buildEnv
, ament-cmake
, ament-cmake-gtest
, pkg-config
, makeWrapper
, rclcpp
, rcl
, rcl-interfaces
, fcl
, gbenchmark
, nova-arm-kinematics
, rmw-cyclonedds-cpp
, rmw-dds-common
}:

# Minimal runtime environment needed to run the benchmarks.
#
# The twistmapper benchmark initialises an rclcpp node (for pluginlib and
# parameter access) and loads the arm_kinematics FCL collision plugin at
# runtime via pluginlib's dlopen mechanism.  Those dynamically-loaded
# libraries are not covered by the binary's RPATH, so we collect them all
# into one merged store path and expose that single directory via
# AMENT_PREFIX_PATH / LD_LIBRARY_PATH through a wrapProgram hook.
let
  runtimeEnv = buildEnv {
    name = "nova-arm-kinematics-benchmark-runtime";
    ignoreCollisions = true;
    paths = [
      # arm_kinematics plugins.xml (for pluginlib) + libarm_kinematics_plugins.so
      nova-arm-kinematics
      # RMW implementation (dlopen'd by rmw_implementation at startup)
      rmw-cyclonedds-cpp
      # DDS common message type-support (dlopen'd by rmw_cyclonedds_cpp)
      rmw-dds-common
      # rcl + rcl_interfaces type-support (dlopen'd when creating rclcpp nodes)
      rcl
      rcl-interfaces
    ];
  };
in

buildRosPackage rec {
  name = "nova-arm-kinematics-benchmark";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-arm-kinematics-benchmark-source";
    path = ./.;
    filter = lib.novaSourceFilter [
    ]
      path;
  };

  nativeBuildInputs = [
    ament-cmake
    ament-cmake-gtest
    pkg-config
    makeWrapper
  ];

  buildInputs = [
    rclcpp
    fcl
    gbenchmark
    nova-arm-kinematics
    rmw-cyclonedds-cpp
  ];

  postInstall = ''
    for bin in $out/lib/arm_kinematics_benchmark/benchmark_* $out/lib/arm_kinematics_benchmark/test_*; do
      if [ ! -e "$bin" ]; then
        continue
      fi
      wrapProgram "$bin" \
        --prefix AMENT_PREFIX_PATH : "${runtimeEnv}:$out" \
        --prefix LD_LIBRARY_PATH   : "${runtimeEnv}/lib" \
        --set-default RMW_IMPLEMENTATION rmw_fastrtps_cpp
    done
  '';

  CMAKE_BUILD_TYPE = "Release";
  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
  ];

  NIX_CFLAGS_COMPILE = [
    "-O3"
#    "-ffast-math"
    "-DNDEBUG"
    "-DEIGEN_NO_DEBUG"
    "-fno-omit-frame-pointer"
    "-g"
  ];

}
