{ lib
, buildRosPackage
, pythonPackages
, rclpy
, sensor-msgs
, vision-msgs
, std-msgs
, cv-bridge
}:

buildRosPackage
{
  name = "detection-overlay";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "nova-detection-overlay-source";
    path = ../../../nav2_autonomous/nova_detection_overlay;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = with pythonPackages; [
    rclpy
    sensor-msgs
    vision-msgs
    std-msgs
    cv-bridge
  ];
}
