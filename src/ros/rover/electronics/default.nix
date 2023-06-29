{ lib
, buildRosPackage
, pythonPackages
, ament-cmake
, rclcpp
, rclpy
, geometry-msgs
, nav-msgs
, core
}:

buildRosPackage {
  name = "electronics";
  buildType = "ament_cmake";

  src = lib.cleanNovaSource [ ] ./.;

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [ rclcpp rclpy geometry-msgs nav-msgs core ];
  propagatedBuildInputs = with pythonPackages; [ coms-utils ];
}
