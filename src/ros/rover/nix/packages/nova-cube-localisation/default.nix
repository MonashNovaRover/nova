{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, nav2-behavior-tree
, pluginlib
, rosidl-default-generators
, behaviortree-cpp-v3
, geometry-msgs
, tf2-ros
, nav2-util
, std-srvs
, launch
, launch-ros
}:

buildRosPackage {
  name = "cube-localisation";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "nova-cube-localisation-source";
    path = ../../../nav2_autonomous/nova_cube_localisation;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [ pluginlib std-srvs rclcpp nav2-behavior-tree behaviortree-cpp-v3 nav2-util tf2-ros geometry-msgs ];
  propagatedBuildInputs = [ launch launch-ros ];
}
