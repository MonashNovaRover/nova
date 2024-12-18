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
  name = "auger_hardware";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "auger-hardware-source";
    path = ../../../hardware_interfaces/auger_hardware;
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
