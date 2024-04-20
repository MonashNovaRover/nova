{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
, nova-python-control
, nova-input-interfaces
}:

buildRosPackage {
  name = "excavation-construction";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "excavation-construction-source";
    path = ../../../excavation_construction;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    nova-python-control
    pythonPackages.jcan
    nova-input-interfaces
  ];
}
