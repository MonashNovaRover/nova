{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, rclcpp-action
, rcpcpp-lifecycle
, rclcpp-components
, std-msgs
, geometry-msgs
, nav2-behavior-tree
, nav-msgs
, nav2-msgs
, behaviortree-cpp-v3
, std_srvs
, nav2_util
, nav2_core
, tf2-ros
, pluginlib
}:

buildRosPackage {
  name = "nova-bt-navigators";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-bt-navigators-source";
    path = ../../../nav2_autonomous/nova-bt-;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake nav2-common ];
  buildInputs = [ behaviortree-cpp-v3 rclcpp rclcpp-action rclcpp-lifecycle 
  nav2-behavior-tree nav-msgs nav2-msgs std-msgs nav2_util geometry-msgs nav2_core tf2-ros pluginlib];
}
