{ lib
, buildRosPackage
, pythonPackages
, ament-cmake
, rclcpp
, rclpy
, geometry-msgs
, nav-msgs
, trajectory-msgs
, nova-core
}:

buildRosPackage {
  name = "electronics";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "electronics-source";
    path = ../../../electronics;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [ rclcpp rclpy geometry-msgs nav-msgs trajectory-msgs nova-core ];
  propagatedBuildInputs = with pythonPackages; [ nova-coms-utils ];
}
