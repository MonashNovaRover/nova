{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, launch
, launch-ros
}:

buildRosPackage {
  name = "science-bringup";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "science-bringup-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [ launch launch-ros ];
}
