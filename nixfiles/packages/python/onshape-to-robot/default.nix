{ lib, buildPythonPackage, fetchPypi, python3Packages
, setup-tools
, numpy
, pandas
, colorama
, commentjson
, numpy-stl
, pybullet
, requests
, sphinx
, sphinx-rtd-theme
, transforms3d
}:

python3Packages.buildPythonApplication rec {
  pname = "onshape-to-robot";
  version = "0.3.26"; # Ensure this is the correct version

  src = fetchPypi {
    inherit pname version;
    sha256 = "0sswn3cm8zs8j11zsiva0sblw2jahzyxz44nmqrv3g1ya8zksfda"; # Replace with the correct hash
  };

  build-system = with python3Packages; [
    setup-tools
  ];

  dependencies = with python3Packages; [
    numpy
    pandas
    colorama
    commentjson
    numpy-stl
    pybullet
    requests
    sphinx
    sphinx-rtd-theme
    transforms3d
  ];

  doCheck = false;

  meta = with lib; {
    description = "A Python library to convert Onshape assemblies into URDF or SDF robots";
    homepage = "https://github.com/Rhoban/onshape-to-robot";
    license = licenses.mit;
    maintainers = with maintainers; [ your_name ];
  };
}