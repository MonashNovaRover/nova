{ lib
, buildRosPackage
, ament-cmake
, std-msgs
, geometry-msgs
, nav-msgs
, launch
, launch-ros
}:

buildRosPackage {
  name = "new-rover-description";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "new-rover-description-source";
    path = ../../../new_rover_description;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  #propagatedBuildInputs = [std-msgs nav-msgs geometry-msgs launch launch-ros ];
}
