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
  name = "electronics";
  buildType = "ament_cmake";

  src = lib.cleanNovaSource [ ] ../../../electronics;

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [ rclcpp rclpy geometry-msgs nav-msgs nova-core ];
  propagatedBuildInputs = with pythonPackages; [ nova-coms-utils ];
}
