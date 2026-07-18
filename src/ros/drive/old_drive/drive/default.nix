{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, nova-blcmd-interfaces
, nova-drive-interfaces
, nova-inputs
, nova-input-interfaces
, libblcmd
, trajectory-msgs
}:

buildRosPackage {
  name = "drive";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "drive-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ]
      path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    nova-blcmd-interfaces
    nova-drive-interfaces
    nova-inputs
    nova-input-interfaces
    trajectory-msgs
    libblcmd
  ];
}
