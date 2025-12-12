{ lib
, buildRosPackage
, ament-cmake
, pluginlib
, rclcpp
, backward-ros
, tf2
, tf2-msgs
, geometry-msgs
, tf2-geometry-msgs
, eigen
, tf2-eigen
, orocos-kdl
, pkg-config
, ament-cmake-gtest
, ament-lint-auto
, transmission-interface
, fcl
, kdl-parser
}:

buildRosPackage rec {
  name = "nova-arm-kinematics";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-arm-kinematics-source";
    path = ./.;
    filter = lib.novaSourceFilter [
    ]
      path;
  };

  nativeBuildInputs = [
    ament-cmake
    pkg-config
    ament-cmake-gtest
    ament-lint-auto
  ];

  buildInputs = [
    pluginlib
    rclcpp
    backward-ros
    tf2
    tf2-msgs
    geometry-msgs
    tf2-geometry-msgs
    eigen
    tf2-eigen
    orocos-kdl
    transmission-interface
    fcl
    kdl-parser
  ];

  propagatedBuildInputs = [ 
    pluginlib
    rclcpp
    backward-ros
    tf2
    tf2-msgs
    geometry-msgs
    tf2-geometry-msgs
    eigen
    tf2-eigen
    orocos-kdl
    transmission-interface
  ];

#  doCheck = true;
#  cmakeFlags = [
#    "-DNIX_DO_CHECK=${if doCheck then "ON" else "OFF"}"
#  ];

  # Added to debug a crazy segfault
#  dontStrip = true;
#  CMAKE_BUILD_TYPE = "RelWithDebInfo";
#  cmakeFlags = [
#    "-DCMAKE_BUILD_TYPE=Debug"
#    "-DCMAKE_CXX_FLAGS=-g"
#  ];
#  cmakeBuildType = "Debug";
#  NIX_CFLAGS_COMPILE = [
#    "-fno-omit-frame-pointer"
#    "-shared"
#  ];
##
#  NIX_LDFLAGS = [
#    "-fno-omit-frame-pointer"
#    "-shared"
#  ];

  CMAKE_BUILD_TYPE = "Release";
  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
  ];

  NIX_CFLAGS_COMPILE = [
    "-O3"
#    "-ffast-math"
    "-DNDEBUG"
    "-DEIGEN_NO_DEBUG"
  ];

}
