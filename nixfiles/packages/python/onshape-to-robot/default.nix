{ lib, fetchPypi, python3Packages, pkg-config }:

with python3Packages;

buildPythonApplication rec {
  pname = "onshape-to-robot";
  version = "0.3.26"; # Ensure this is the correct version

  src = fetchPypi {
    inherit pname version;
    hash = "sha256-aMxfZw1B/J1NgRbbUXRwjM2hEbUlTXfVIt9r2dVtOuY="; # Replace with the correct hash
  };

  build-system = [
    setuptools
    wheel
  ];

  propagatedBuildInputs = [
    numpy
    pybullet
    requests
    commentjson
    colorama
    numpy-stl
    transforms3d
  ];

  nativeBuildInputs = [
    pkg-config
  ];

  meta = with lib; {
    description = "A Python library to convert Onshape assemblies into URDF or SDF robots";
    homepage = "https://github.com/Rhoban/onshape-to-robot";
    license = licenses.mit;
    maintainers = with maintainers; [ your_name ];
  };
}
