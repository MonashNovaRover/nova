{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
}:

buildRosPackage {
  name = "python-control-old";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "python-control-old-source";
    path = ../../../python_control_old;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    pythonPackages.jcan
  ];

}
