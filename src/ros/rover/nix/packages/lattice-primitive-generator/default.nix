{ lib
, buildRosPackage
, pythonPackages
}:

buildRosPackage {
  name = "lattice-primitive-generator";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "lattice-primitive-generator-source";
    path = ../../../lattice_primitive_generator;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    pythonPackages.numpy
    pythonPackages.matplotlib
    pythonPackages.rtree
  ];

}
