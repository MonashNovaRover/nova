{ lib
, pkg-config
, buildRosPackage
, ament-cmake
, ament-cmake-gtest
, ament-lint-auto
, gtest
, rclcpp
, joy
, nova-input-interfaces
, nova-interfaces
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
  name = "teleop_arm_joy";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "teleop-arm-source";
    path = ../../../teleop_arm_joy;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [
    ament-cmake
    pkg-config
    ament-cmake-gtest
    ament-lint-auto
    gtest
  ];

  buildInputs = [
    control-msgs
    rclcpp
    std-srvs
    generate-parameter-library
    realtime-tools
    geometry-msgs
    nova-input-interfaces
    nova-interfaces
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

  # Enable running tests during build
  doCheck = true;
}
