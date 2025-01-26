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
, backward-ros
, nav-msgs
, realtime-tools
, tf2
, tf2-msgs
, geometry-msgs
, nova-interfaces
}:

buildRosPackage {
  name = "nova-arm-controller";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova_arm_controller-source";
    path = ../../../../controllers/nova_arm_controller;
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
    backward-ros
    nav-msgs
    realtime-tools
    tf2
    tf2-msgs
    geometry-msgs
    nova-interfaces
  ];
}

