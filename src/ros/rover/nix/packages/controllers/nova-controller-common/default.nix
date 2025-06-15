{ lib
, buildRosPackage
, ament-cmake
}:

buildRosPackage {
  name = "nova-controller-common";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-controller-common-source";
    path = ../../../../controllers/nova_controller_common;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
}
