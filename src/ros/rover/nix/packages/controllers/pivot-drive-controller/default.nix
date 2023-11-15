{ lib
, buildRosPackage
, ament-cmake
}:

buildRosPackage {
  name = "pivot-drive-controller";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "pivot_drive_controller-source";
    path = ../../../../controllers/pivot_drive_controller;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
}
