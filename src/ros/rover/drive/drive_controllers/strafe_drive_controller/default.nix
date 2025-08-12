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
, backward-ros
, generate-parameter-library
, nav-msgs
, realtime-tools
, tf2
, tf2-msgs
, geometry-msgs
, nova-controller-common
, nova-drive-controller-base
}:

buildRosPackage {
  name = "strafe-controller";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "strafe_drive_controller-source";
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
    backward-ros
    generate-parameter-library
    nav-msgs
    realtime-tools
    tf2
    tf2-msgs
    geometry-msgs
    nova-controller-common
    nova-drive-controller-base
  ];
}
