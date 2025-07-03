{ lib
, buildRosPackage
, ament-cmake
}:

buildRosPackage {
  name = "nova-cpp";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-cpp-source";
    path = ../../../nova_cpp;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
}
