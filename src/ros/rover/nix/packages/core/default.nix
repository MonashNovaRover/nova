{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
, geometry-msgs
, nav-msgs
, trajectory-msgs
, launch
, launch-ros
, xacro
, robot-state-publisher
, joint-state-publisher
, controller-manager
, ros2-control
# TODO: Replace with gazebo harmonic, after migrating the launch files
# https://gazebosim.org/docs/harmonic/migrating_gazebo_classic_ros2_packages/
#, gazebo-ros
#, gazebo-ros2-control
, ros2-controllers
, pluginlib
, robot-localization
, nova-behavior-tree
, nova-costmap-2d
, image-view
, navigation2
, depthai-ros
}:

buildRosPackage rec {
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
      ros2-controllers
      pluginlib
      image-view
      robot-localization
      navigation2
      depthai-ros;
  };

}
