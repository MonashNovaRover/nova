{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, python3Packages
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
    python3Packages.jcan
  ];

}
