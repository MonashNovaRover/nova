{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
}:

buildRosPackage {
  name = "nova-camera-msgs";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-camera-msgs-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
}
