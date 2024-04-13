{ lib
, buildRosPackage
, pythonPackages
, ament-cmake
, rclcpp
, rclpy
, geometry-msgs
, nav-msgs
, trajectory-msgs
, nova-interfaces
}:

buildRosPackage {
  name = "science";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "science-source";
    path = ../../../science;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [ rclcpp rclpy geometry-msgs nav-msgs trajectory-msgs nova-interfaces ];

  propagatedBuildInputs = with pythonPackages; [
    jcan
    nova-coms-utils
  ];
}
