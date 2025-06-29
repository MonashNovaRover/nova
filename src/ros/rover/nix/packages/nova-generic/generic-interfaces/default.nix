{ lib,
  buildRosPackage,
  ament-cmake,
  rosidl-default-generators,
  std-msgs,
  geometry-msgs,
  nav-msgs,
  action-msgs,
  sensor-msgs,
}:

buildRosPackage {
  name = "generic-interfaces";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "generic-interfaces-source";
    path = ../../../nova_generic/generic_interfaces;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [
    std-msgs
    nav-msgs
    geometry-msgs
    action-msgs
    sensor-msgs
  ];
}
