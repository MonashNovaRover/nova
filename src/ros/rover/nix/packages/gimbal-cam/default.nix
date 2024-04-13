{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
, nova-input-msgs
}:

buildRosPackage {
  name = "gimbal-cam";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "gimabl-cam-source";
    path = ../../../gimbal_cam;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    nova-input-msgs
    pythonPackages.jcan
  ];
}
