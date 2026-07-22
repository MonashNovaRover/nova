{ lib
, buildRosPackage
, ament-cmake
}:

buildRosPackage {
  name = "rover-description";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "rover-description-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ ];
}
