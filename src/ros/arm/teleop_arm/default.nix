{ lib
, pkg-config
, buildRosPackage
, ament-cmake
, rclcpp
, joy
, nova-interfaces
, geometry-msgs
, sensor-msgs
, pluginlib
, teleop-modular-node
, teleop-modular-twist
, nova-joint-space-control-mode
}:

buildRosPackage {
  name = "teleop-arm";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "teleop-arm-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake pkg-config ];

  buildInputs = [
  ];

  propagatedBuildInputs = [
    joy
    geometry-msgs
    nova-interfaces
    sensor-msgs
    pluginlib
    teleop-modular-twist
    teleop-modular-node
    nova-joint-space-control-mode
];
}
