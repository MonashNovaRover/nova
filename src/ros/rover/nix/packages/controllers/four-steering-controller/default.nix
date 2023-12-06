{ lib
, buildRosPackage
, ament-cmake
, control-msgs
, controller-interface
, hardware-interface
, pluginlib
, rclcpp
, rclcpp-lifecycle
, std-srvs
, generate-parameter-library
, rcpputils
, admittance-controller
, four-wheel-steering-msgs
}:

buildRosPackage {
  name = "four-steering-controller";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "four_steering_controller-source";
    path = ../../../../controllers/ros2_controllers/four_steering_controller;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    control-msgs
    controller-interface
    hardware-interface
    pluginlib
    rclcpp
    rclcpp-lifecycle
    std-srvs
    generate-parameter-library
    four-wheel-steering-msgs
  ];
}
