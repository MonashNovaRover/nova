{ lib
, buildPythonPackage
, fetchPypi
, setuptools
}:

buildPythonPackage rec {
  pname = "linuxpy";
  version = "0.20.0";
  pyproject = true;

  src = fetchPypi {
    inherit pname version;
    hash = "sha256-mNWmzl52GEZUEL3q8cP59qxMduG1ijgsvGoD5ddSG94=";
  };

  nativeBuildInputs = [ setuptools ];

  pythonImportsCheck = [ "linuxpy" ];
}
