{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
, nova-input-interfaces
}:

buildRosPackage {
  name = "gimbal-cam";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "gimabl-cam-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    nova-input-interfaces
    pythonPackages.jcan
  ];
}
