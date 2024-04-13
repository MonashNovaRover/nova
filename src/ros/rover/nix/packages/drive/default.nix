{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, nova-blcmd-interfaces
, nova-drive-interfaces
, nova-inputs
, nova-input-msgs
, libblcmd
}:

buildRosPackage {
  name = "drive";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "drive-source";
    path = ../../../drive/drive;
    filter = lib.novaSourceFilter [ ]
      path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    nova-blcmd-interfaces
    nova-drive-interfaces
    nova-inputs
    nova-input-msgs
    libblcmd
  ];
}
