{ lib
, buildRosPackage
, python3Packages
, ament-cmake
}:

buildRosPackage {
  name = "lattice-primitive-generator";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "lattice-primitive-generator-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  propagatedBuildInputs = [
    python3Packages.numpy
    python3Packages.matplotlib
    python3Packages.rtree
  ];

}