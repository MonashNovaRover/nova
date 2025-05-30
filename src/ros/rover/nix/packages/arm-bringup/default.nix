{ lib
, buildRosPackage
, ament-cmake
, launch
, launch-ros
, xacro
, tf2-tools
, robot-state-publisher
, controller-manager
, ros2-control
, ros-gz
, ros2-controllers
, nova-rover-description
, nova-gazebo
, nova-auto-typing
}:

buildRosPackage {
  name = "arm-bringup";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "arm-bringup-source";
    path = ../../../arm_bringup;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  
  propagatedBuildInputs = [
    launch
    launch-ros
  ];

  passthru.workspacePackages = {
    inherit
      xacro
      robot-state-publisher
      controller-manager
      ros2-control
      ros-gz
      ros2-controllers
      nova-rover-description
      nova-gazebo
      nova-auto-typing
      tf2-tools;
  };
}
