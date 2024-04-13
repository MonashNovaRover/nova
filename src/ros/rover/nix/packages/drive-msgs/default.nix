{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
, geometry-msgs
}:

buildRosPackage {
  name = "drive-msgs";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "drive-msgs-source";
    path = ../../../drive_msgs;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [ std-msgs geometry-msgs ];
}