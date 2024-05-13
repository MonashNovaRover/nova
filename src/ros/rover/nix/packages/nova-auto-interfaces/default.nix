{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
, geometry-msgs
, nav-msgs
}:

buildRosPackage {
  name = "nova-auto-interfaces";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-auto-interfaces-source";
    path = ../../../nav2_autonomous/nova_auto_interfaces;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [
    std-msgs
    nav-msgs
    geometry-msgs
  ];
}
