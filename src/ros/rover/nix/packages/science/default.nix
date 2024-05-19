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
, nova-input-interfaces
, nova-python-control
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
    pymodbus
  ] ++
  [
    nova-python-control
    nova-input-interfaces
  ];
}
