{ lib
, buildRosPackage
, ament-cmake
, controller-interface
, hardware-interface
, pluginlib
, rclcpp
, rclcpp-lifecycle
, std-srvs
, generate-parameter-library
, backward-ros
, nav-msgs
, realtime-tools
, tf2
, tf2-msgs
, geometry-msgs
, nova-controller-common
, nova-drive-controller-base
}:

buildRosPackage {
  name = "diff-drive-controller";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "diff_drive_controller-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
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
    nova-controller-common
    nova-drive-controller-base
  ];
}
