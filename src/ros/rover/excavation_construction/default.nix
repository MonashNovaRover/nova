{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
, nova-python-control
, nova-input-interfaces
, nova-python-control-old
, nova-python-control2
}:

buildRosPackage {
  name = "excavation-construction";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "excavation-construction-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    nova-python-control
    nova-python-control-old
    pythonPackages.jcan
    nova-input-interfaces
    nova-python-control2
  ];
}
