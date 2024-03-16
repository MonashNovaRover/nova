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
, launch
, launch-ros
}:

buildRosPackage
{
  name = "cube-localisation";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "nova-cube-localisation-source";
    path = ../../../nav2_autonomous/nova_cube_localisation;
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
    launch
    launch-ros
  ];
}
