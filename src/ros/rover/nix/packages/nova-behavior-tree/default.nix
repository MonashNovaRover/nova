{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, nav2-behavior-tree
, pluginlib
, rosidl-default-generators
, behaviortree-cpp
, std-srvs
, launch
, launch-ros
}:

buildRosPackage {
  name = "behavior-tree";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-behavior-tree-source";
    path = ../../../nav2_autonomous/nova_behavior_tree;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [ pluginlib std-srvs rclcpp nav2-behavior-tree behaviortree-cpp ];
  propagatedBuildInputs = [ launch launch-ros ];
}
