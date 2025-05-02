{ lib
, buildRosPackage
, pythonPackages
, std-srvs
, sensor-msgs
, rclpy
, tf2-ros
, visualization-msgs
, vision-msgs
, geometry-msgs
, std-msgs
, vision-opencv
, image-geometry
, launch
, launch-ros
}:

buildRosPackage
{
  name = "object-localisation";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "nova-object-localisation-source";
    path = ../../../nav2_autonomous/nova_object_localisation;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    std-srvs
    sensor-msgs
    rclpy
    tf2-ros
    visualization-msgs
    vision-msgs
    geometry-msgs
    std-msgs
    vision-opencv
    image-geometry
    launch
    launch-ros
  ];
}
