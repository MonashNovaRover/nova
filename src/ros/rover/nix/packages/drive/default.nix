{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, nova-blcmd-interfaces
, nova-drive-msgs
, nova-core
, libblcmd
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

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    nova-blcmd-interfaces
    nova-drive-msgs
    nova-core
    libblcmd
  ];
}
