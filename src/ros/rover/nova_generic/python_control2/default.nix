{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, python3Packages
, teleop-modular-python-utils
, nova-interfaces
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
    python3Packages.jcan
    teleop-modular-python-utils
    nova-interfaces
  ];

}
