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
}:

buildRosPackage {
  name = "object-detection";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "object-detection-source";
    path = ../../../object_detection;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators std-msgs sensor-msgs nav-msgs opencv tf2-ros];

  propagatedBuildInputs = [
    rclcpp
    opencv
  ];
}
