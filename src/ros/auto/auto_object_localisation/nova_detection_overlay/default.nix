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
    path = ./.;
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
