{ lib
, buildRosPackage
, ament-cmake

, launch
, launch-ros
}:

buildRosPackage {
  name = "nova-gazebo";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-gazebo-source";
    path = ../../../nova_gazebo;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ launch launch-ros ];
}
