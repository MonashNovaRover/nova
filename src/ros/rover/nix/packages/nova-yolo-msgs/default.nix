{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
, geometry-msgs
}:

buildRosPackage {
  name = "yolo-msgs";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-yolo-msgs";
    path = ../../../nova_yolo_ros/yolo_msgs;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [std-msgs geometry-msgs];
}
