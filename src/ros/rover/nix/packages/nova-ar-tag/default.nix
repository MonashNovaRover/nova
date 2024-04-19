{ lib
, buildRosPackage
, pythonPackages
, rclpy
, aruco-opencv-msgs
, visualization-msgs
, geometry-msgs
, std-msgs
, launch
, launch-ros
}:

buildRosPackage
{
  name = "ar-tag";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "nova-ar-tag-source";
    path = ../../../nav2_autonomous/nova_ar_tag;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    aruco-opencv-msgs
    visualization-msgs
    geometry-msgs
    std-msgs
    launch
    launch-ros
  ];
}
