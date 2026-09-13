{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, python3Packages
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
    python3Packages.jcan
  ];
}
