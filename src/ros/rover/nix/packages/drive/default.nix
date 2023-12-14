{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, nova-core
, libcanmd
, jcan
, pkg-config
}:

buildRosPackage {
  name = "control";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "drive-source";
    path = ../../../drive;
    filter = lib.novaSourceFilter [ ]
      path;
  };

  nativeBuildInputs = [ ament-cmake pkg-config ];

  buildInputs = [
    rclcpp
    nova-core
    libcanmd
    jcan
  ];
}
