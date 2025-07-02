{ lib
, buildRosPackage
, pythonPackages
, sensor-msgs
, rclpy
, std-msgs
, launch
, launch-ros
}:

buildRosPackage
{
  name = "pointcloud-filter";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "nova-pointcloud-filter-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    sensor-msgs
    rclpy
    std-msgs
    launch
    launch-ros
  ];
}
