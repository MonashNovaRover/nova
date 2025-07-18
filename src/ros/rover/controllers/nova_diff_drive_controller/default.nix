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
  name = "nova-diff-drive-controller";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova_diff_drive_controller-source";
    path = ./.;
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
