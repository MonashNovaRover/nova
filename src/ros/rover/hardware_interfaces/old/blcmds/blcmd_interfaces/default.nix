{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
}:

buildRosPackage {
  name = "blcmd-interfaces";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "blcmd-interfaces-source";
    path = ../../../blcmds/blcmd_interfaces;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [ std-msgs ];
}