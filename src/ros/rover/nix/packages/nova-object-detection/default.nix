{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, rosidl-default-generators
, std-msgs
, sensor-msgs
, nav-msgs
, opencv
, tf2-ros
, pythonPackages
}:

buildRosPackage {
  name = "nova-object-detection";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-object-detection-source";
    path = ../../../object_detection;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ 
    ament-cmake 
    rosidl-default-generators 
    std-msgs 
    sensor-msgs 
    nav-msgs 
    opencv 
    tf2-ros
  ];

  propagatedBuildInputs = [
    rclcpp
    opencv
    pythonPackages.ultralytics
  ];
}
