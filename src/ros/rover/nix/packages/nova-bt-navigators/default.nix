{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, rclcpp-action
, rclcpp-lifecycle
, rclcpp-components
, std-msgs
, geometry-msgs
, nav2-behavior-tree
, nav-msgs
, nav2-msgs
, behaviortree-cpp
, std-srvs
, nav2-util
, nav2-core
, nav2-common
, tf2-ros
, pluginlib
, nova-auto-interfaces
, robot-localization
, geographic-msgs
}:

buildRosPackage {
  name = "nova-bt-navigators";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-bt-navigators-source";
    path = ../../../nav2_autonomous/nova_bt_navigators;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake nav2-common ];
  buildInputs = [
    behaviortree-cpp
    rclcpp
    rclcpp-action
    rclcpp-lifecycle
    nav2-behavior-tree
    nav-msgs
    nav2-msgs
    std-msgs
    std-srvs
    nav2-util
    geometry-msgs
    nav2-core
    tf2-ros
    pluginlib
    nova-auto-interfaces
    robot-localization
    geographic-msgs
  ];
}
