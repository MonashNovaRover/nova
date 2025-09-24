{ lib
, buildRosPackage
, ament-cmake
, launch
, launch-ros
}:

buildRosPackage {
  name = "drive-bringup";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "drive-bringup-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  
  propagatedBuildInputs = [
    launch
    launch-ros
  ];
}
