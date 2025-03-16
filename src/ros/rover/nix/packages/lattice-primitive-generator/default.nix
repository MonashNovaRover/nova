{ lib
, buildRosPackage
, pythonPackages
, ament-cmake
}:

buildRosPackage {
  name = "lattice-primitive-generator";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "lattice-primitive-generator-source";
    path = ../../../lattice_primitive_generator;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  propagatedBuildInputs = [
    pythonPackages.numpy
    pythonPackages.matplotlib
    pythonPackages.rtree
  ];

}