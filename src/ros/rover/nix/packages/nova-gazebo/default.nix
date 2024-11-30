{ lib
, buildRosPackage
, ament-cmake
, launch
, launch-ros
, stdenv
, leo-gz-worlds
}:

buildRosPackage rec {
  name = "nova-gazebo";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-gazebo-source";
    path = ../../../nova_gazebo;
    filter = lib.novaSourceFilter [ "!worlds/**" ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ launch launch-ros leo-gz-worlds ];
}
