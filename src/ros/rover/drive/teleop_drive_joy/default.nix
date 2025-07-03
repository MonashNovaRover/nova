{ lib
, pkg-config
, buildRosPackage
, ament-cmake
, nova-drive-interfaces
, nova-input-interfaces
, nova-interfaces
, nova-cpp
, rclcpp
, joy
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
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake pkg-config];

  buildInputs = [
    nova-drive-interfaces
    nova-input-interfaces
    nova-interfaces
    nova-cpp
    control-msgs
    rclcpp
    std-srvs
    generate-parameter-library
    realtime-tools
    geometry-msgs
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
