{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
, nav-msgs
, launch
, launch-ros
}:

buildRosPackage {
  name = "core";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "core-source";
    path = ../../../core;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [ std-msgs nav-msgs ];
  propagatedBuildInputs = [ launch launch-ros ];
}
