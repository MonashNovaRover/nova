{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, launch
, launch-ros
, joint-state-publisher
, xacro
, aruco-opencv
, aruco-opencv-msgs
, robot-state-publisher
, controller-manager
, ros2-control
, ros-gz
, ros2-controllers
, pluginlib
, robot-localization
, image-view
, navigation2
, depthai-ros
# , rtabmap-ros
, nova-behavior-tree
, nova-costmap-2d
, nova-pointcloud-filter
, nova-rover-description
, nova-gazebo
, nova-auto-interfaces
, nova-bt-navigators
, rviz-imu-plugin
, imu-transformer
, nova-pivot-drive-controller
, nova-auger-controller
}:

buildRosPackage {
  name = "nova-bringup";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-bringup-source";
    path = ../../../nova_bringup;
    filter = lib.novaSourceFilter [ ] path;
  };

  passthru.workspacePackages = {
    inherit
      xacro
      robot-state-publisher
      controller-manager
      ros2-control
      ros-gz
      ros2-controllers
      aruco-opencv
      aruco-opencv-msgs
      pluginlib
      robot-localization
      image-view
      navigation2
      depthai-ros
      # rtabmap-ros
      nova-behavior-tree
      nova-costmap-2d
      nova-pointcloud-filter
      nova-rover-description
      nova-gazebo
      nova-auto-interfaces
      nova-bt-navigators
      rviz-imu-plugin
      nova-pivot-drive-controller
      imu-transformer;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [ launch launch-ros joint-state-publisher ];
}
