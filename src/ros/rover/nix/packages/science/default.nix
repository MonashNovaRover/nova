{ lib
, buildRosPackage
, pythonPackages
, ament-cmake
, rclcpp
, rclpy
, geometry-msgs
, nav-msgs
, nova-core
}:

buildRosPackage {
  name = "science";
  buildType = "ament_cmake";

  src = lib.cleanNovaSource [ ] ../../../science;

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [ rclcpp rclpy geometry-msgs nav-msgs nova-core ];
  propagatedBuildInputs = with pythonPackages; [ nova-coms-utils ];
}
