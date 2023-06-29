{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
}:

buildRosPackage {
  name = "camera-msgs";
  buildType = "ament_cmake";

  src = lib.cleanNovaSource [ ] ./.;

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
}
