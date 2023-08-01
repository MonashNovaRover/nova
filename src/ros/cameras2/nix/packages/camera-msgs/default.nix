{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
}:

buildRosPackage {
  name = "camera-msgs";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "camera-msgs-source";
    path = ../../../camera_msgs;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
}
