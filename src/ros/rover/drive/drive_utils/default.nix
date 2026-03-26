{ lib
, buildRosPackage
, pythonPackages
, ament-cmake
, python3Packages
, rclcpp
, rclpy
, geometry-msgs
, nav-msgs
, nova-drive-interfaces
}:

buildRosPackage {
  name = "drive-utils";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "drive-utils-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [ rclcpp rclpy geometry-msgs nav-msgs ];

  propagatedBuildInputs = with pythonPackages; [
    nova-drive-interfaces
    geometry-msgs
  ];
}
