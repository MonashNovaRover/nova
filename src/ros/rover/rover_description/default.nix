{ lib, 
  buildRosPackage, 
  ament-cmake, 
  topic-based-ros2-control, 
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
  propagatedBuildInputs = [ topic-based-ros2-control ];
}
