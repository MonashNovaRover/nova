{ lib
, pkg-config
, buildRosPackage
, ament-cmake
, rclcpp
, joy
, nova-core
, geometry-msgs
, sensor-msgs
, generate-parameter-library
, pluginlib
, rclcpp-components
, realtime-tools
, controller-manager-msgs
, control-msgs
, std-msgs
, std-srvs
, nav-msgs
}:

buildRosPackage {
  name = "teleop_drive_joy";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "teleop-drive-source";
    path = ../../../teleop_drive_joy;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake pkg-config];

  buildInputs = [
    control-msgs
    rclcpp
    std-srvs
    generate-parameter-library
    realtime-tools
    geometry-msgs
    nova-core
    sensor-msgs
    pluginlib
    rclcpp-components
    realtime-tools
    controller-manager-msgs
    control-msgs
    std-msgs
    nav-msgs
  ];

  propagatedBuildInputs = [ joy ];
}
