{ lib, 
  buildRosPackage, 
  ament-cmake, 
  rosidl-default-generators,
  # Add dependencies here, e.g.:
  #std-msgs,
}:

buildRosPackage {
  name = "science-interfaces";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "science-interfaces-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [
    # Add dependencies here, e.g.:
#    std-msgs
  ];
}
