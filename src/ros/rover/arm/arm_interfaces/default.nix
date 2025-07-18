{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
, geometry-msgs
}:

buildRosPackage {
  name = "arm-interfaces";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "arm-interfaces-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [ std-msgs geometry-msgs ];
}