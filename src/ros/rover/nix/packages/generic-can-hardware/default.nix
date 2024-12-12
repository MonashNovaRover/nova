{ lib
, pkg-config
, buildRosPackage
, ament-cmake
, rclpy
, rclcpp
, rclcpp-lifecycle
, hardware-interface
, pluginlib
, jcan
}:

buildRosPackage {
  name = "generic_can_hardware";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "generic-can-hardware-source";
    path = ../../../hardware_interfaces/generic_can_hardware;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake pkg-config];

  buildInputs = [
    rclcpp
    rclcpp-lifecycle
    hardware-interface
    jcan
  ];
}
