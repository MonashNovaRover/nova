{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, python3Packages
, nova-python-control
, nova-input-interfaces
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
    python3Packages.jcan
    nova-input-interfaces
    nova-python-control2
  ];
}
