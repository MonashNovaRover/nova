{ lib
, buildRosPackage
, pythonPackages
, python3Packages
, opencv4
, sensor-msgs
, visualization-msgs
, rclpy
, std-msgs
, cv-bridge
, std-srvs
, message-filters
, nova-yolo-msgs
, ament-cmake 
, rosidl-default-generators
}:

buildRosPackage
{
  name = "yolo-ros";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "nova-yolo-ros";
    path = ../../../nova_yolo_ros/yolo_ros;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    pythonPackages.ultralytics
    python3Packages.typing-extensions
    opencv4
    pythonPackages.super-gradients
    pythonPackages.lap
    sensor-msgs
    visualization-msgs
    rclpy
    python3Packages.pytorch
    python3Packages.numpy
    cv-bridge
    std-msgs
    std-srvs
    message-filters
    nova-yolo-msgs
  ];
}
