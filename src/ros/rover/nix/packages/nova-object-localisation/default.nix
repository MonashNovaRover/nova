{ lib, 
  buildRosPackage, 
  std-srvs, 
  sensor-msgs, 
  rclpy, 
  tf2-ros, 
  visualization-msgs, 
  vision-msgs, 
  geometry-msgs, 
  std-msgs, 
  vision-opencv, 
  image-geometry, 
  launch, 
  launch-ros, 
  python3Packages, 
  opencv4, 
  cv-bridge, 
  message-filters, 
  yolo-msgs, 
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
    python3Packages.typing-extensions
    python3Packages.pytorch
    python3Packages.numpy
    python3Packages.ultralytics
    python3Packages.super-gradients
    python3Packages.lap
    opencv4 
    cv-bridge 
    message-filters 
    yolo-msgs 
  ];
}
