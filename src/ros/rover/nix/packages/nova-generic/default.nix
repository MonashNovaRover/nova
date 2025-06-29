{ lib
, writeShellApplication
, buildRosPackage
, rclcpp
, rclpy
, ament-cmake
, nova-interfaces
, pythonPackages
, generate-parameter-library
, generate-parameter-library-py
}:

buildRosPackage {
  name = "nova-generic";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-generic-source";
    path = ../../../nova_generic;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [ rclcpp rclpy nova-interfaces ];

  propagatedBuildInputs = with pythonPackages; [
    rclpy
    jcan
    generate-parameter-library
    generate-parameter-library-py
  ];

}
