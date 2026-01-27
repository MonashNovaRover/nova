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
  name = "qcmd_hardware";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "qcmd-hardware-source";
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
