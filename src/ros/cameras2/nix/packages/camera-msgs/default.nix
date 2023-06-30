{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
}:

buildRosPackage {
  name = "camera-msgs";
  buildType = "ament_cmake";

  src = lib.cleanNovaSource [ ] ../../../camera_msgs;

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
}
