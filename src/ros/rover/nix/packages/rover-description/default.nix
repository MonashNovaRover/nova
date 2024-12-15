{ lib
, buildRosPackage
, ament-cmake
, gz-attach-links
}:

buildRosPackage {
  name = "rover-description";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "rover-description-source";
    path = ../../../rover_description;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ gz-attach-links ];
}
