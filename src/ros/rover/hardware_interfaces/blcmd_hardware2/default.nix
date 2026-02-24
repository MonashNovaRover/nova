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
, transmission-interface
}:

buildRosPackage {
  name = "blcmd_hardware2";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "blcmd-hardware2-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake pkg-config];

  buildInputs = [
    rclcpp
    rclcpp-lifecycle
    hardware-interface
    jcan
    transmission-interface
  ];
}
