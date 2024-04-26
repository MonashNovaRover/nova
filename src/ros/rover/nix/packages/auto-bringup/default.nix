{ lib
, buildRosPackage
, ament-cmake
, launch
, launch-ros
, xacro
, robot-state-publisher
, controller-manager
, ros2-control
, gazebo-ros
, gazebo-ros2-control
, gazebo-ros-pkgs
, ros2-controllers
, pluginlib
, robot-localization
, image-view
, navigation2
, depthai-ros
, rtabmap-ros
, nova-behavior-tree
, nova-costmap-2d
, nova-pointcloud-filter
, nova-rover-description
}:

buildRosPackage rec {
  name = "auto-bringup";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "auto-bringup-source";
    path = ../../../auto_bringup;
  };
  
  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ launch launch-ros ];

  passthru.workspacePackages = {
    inherit
      xacro
      robot-state-publisher
      controller-manager
      ros2-control
      gazebo-ros
      gazebo-ros2-control
      gazebo-ros-pkgs
      ros2-controllers
      pluginlib
      robot-localization
      image-view
      navigation2
      depthai-ros
      rtabmap-ros
      nova-behavior-tree
      nova-costmap-2d
      nova-pointcloud-filter
      nova-rover-description;
  };
}
