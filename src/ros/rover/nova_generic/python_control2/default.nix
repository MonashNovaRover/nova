{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
, teleop-modular-python-utils
}:

buildRosPackage {
  name = "python-control2";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "python-control2-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    pythonPackages.jcan
    teleop-modular-python-utils
  ];

}
