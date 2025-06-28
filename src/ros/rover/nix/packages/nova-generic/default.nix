{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
}:

buildRosPackage {
  name = "nova-generic";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "nova-generic-source";
    path = ../../../nova_generic;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    pythonPackages.jcan
  ];

}
