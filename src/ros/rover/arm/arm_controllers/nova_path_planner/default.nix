{ lib
, buildRosPackage
, ament-cmake
, fcl
, nova-arm-kinematics
, control-msgs
, controller-interface
, hardware-interface
, pluginlib
, rclcpp
, rclcpp-action
, rclcpp-lifecycle
, std-srvs
, generate-parameter-library
, rcpputils
, backward-ros
, realtime-tools
, tf2
, tf2-eigen
, tf2-msgs
, geometry-msgs
, tf2-geometry-msgs
, nova-interfaces
}:

buildRosPackage {
  name = "nova-path-planner";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova_path_planner-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    nova-arm-kinematics
    control-msgs
    controller-interface
    fcl
    hardware-interface
    pluginlib
    rclcpp
    rclcpp-action
    rclcpp-lifecycle
    std-srvs
    generate-parameter-library
    backward-ros
    realtime-tools
    tf2
    tf2-eigen
    tf2-msgs
    geometry-msgs
    tf2-geometry-msgs
    nova-interfaces
  ];
}
