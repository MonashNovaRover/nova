{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
}:

buildRosPackage {
  name = "python-control";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "python-control-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    pythonPackages.jcan
  ];

}
