{ lib
, writeShellApplication
, buildRosPackage
, rclcpp
, rclpy
, ament-cmake
, nova-generic-interfaces
, pythonPackages
, generate-parameter-library
, generate-parameter-library-py
}:

buildRosPackage {
  name = "generic-can-nodes";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "generic-can-nodes-source";
    path = ../../../../nova_generic/generic_can_nodes;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [ rclcpp rclpy nova-generic-interfaces ];

  propagatedBuildInputs = with pythonPackages; [
    rclpy
    jcan
    generate-parameter-library
    generate-parameter-library-py
  ];

}
