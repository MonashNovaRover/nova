{ lib
, writeShellApplication
, buildRosPackage
, ament-cmake
, rclcpp
, rclpy
, pythonPackages
, nova-cmd-interfaces
, libcanmd
}:

buildRosPackage {
  name = "cmd-utils";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "cmd-utils-source";
    path = ../../../cmds/cmd_utils;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    libcanmd
  ];

  propagatedBuildInputs = [
    rclpy
    nova-cmd-interfaces
  ];
}
