{ buildPythonPackage
, fetchPypi
, setuptools
}:

buildPythonPackage rec {
  pname = "pygobject-stubs";
  version = "2.6.0";
  format = "pyproject";

  src = fetchPypi {
    pname = "PyGObject-stubs";
    inherit version;
    hash = "sha256-KH2awGAxJH0rTEWmQtjbhdaAqCWN3JD0m7skBou+Wpw=";
  };

  nativeBuildInputs = [ setuptools ];
}
