{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
, nav-msgs
, trajectory-msgs
, launch
, launch-ros
, xacro
, robot-state-publisher
, joint-state-publisher
, controller-manager
, ros2-control
, gazebo-ros
, gazebo-ros2-control
, ros2-controllers
, pluginlib
, gazebo-ros-pkgs
, robot-localization
, nova-costmap-2d
, navigation2
}:

buildRosPackage {
  name = "core";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "core-source";
    path = ../../../core;
    filter = lib.novaSourceFilter [ "!worlds/**" ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [ std-msgs nav-msgs trajectory-msgs ];
  propagatedBuildInputs = [ launch launch-ros ];

  passthru.workspacePackages = {
    inherit
      xacro
      robot-state-publisher
      joint-state-publisher
      controller-manager
      ros2-control
      gazebo-ros
      gazebo-ros2-control
      ros2-controllers
      pluginlib
      robot-localization
      gazebo-ros-pkgs
      nova-costmap-2d
      navigation2;
  };
}
