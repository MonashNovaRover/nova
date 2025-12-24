{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
}:

buildRosPackage {
  name = "nova-cameras3-msgs";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-cameras3-msgs-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
}
