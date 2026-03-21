{ lib
, buildRosPackage
, ament-cmake
, xacro
, launch
, launch-ros
, leo-gz-worlds
}:

buildRosPackage rec {
  name = "nova-gazebo";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-gazebo-source";
    path = ./.;
    filter = lib.novaSourceFilter [ "!worlds/**" ] path;
  };

  nativeBuildInputs = [ ament-cmake xacro ];
  propagatedBuildInputs = [ launch launch-ros leo-gz-worlds ];
}
