{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
}:

buildRosPackage {
  name = "input-msgs";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "input-msgs-source";
    path = ../../../input_interfaces;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [ std-msgs ];
}