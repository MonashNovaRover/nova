{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
, nova-python-control
, nova-input-interfaces
, nova-python-control-old
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
    nova-python-control-old
    pythonPackages.jcan
    nova-input-interfaces
  ];
}
