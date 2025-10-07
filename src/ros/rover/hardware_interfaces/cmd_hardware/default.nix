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
  name = "cmd_hardware";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "cmd-hardware-source";
    path = ./.;
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
