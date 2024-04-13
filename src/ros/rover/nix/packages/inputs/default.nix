{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, nova-input-msgs
, systemd
}:

buildRosPackage {
  name = "inputs";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "inputs-source";
    path = ../../../inputs;
    filter = lib.novaSourceFilter [ ]
      path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    nova-input-msgs
    systemd
  ];
}
