{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
}:

buildRosPackage {
  name = "cmd-interfaces";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "cmd-interfaces-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [ std-msgs ];
}