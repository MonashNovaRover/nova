{ buildPythonPackage
, fetchPypi
, setuptools
}:

buildPythonPackage rec {
  pname = "pygobject-stubs";
  version = "2.4.0";
  format = "pyproject";

  src = fetchPypi {
    pname = "PyGObject-stubs";
    inherit version;
    hash = "sha256-ZHA9WE1lrzdsWa2v8NXzmd0k4oP0SdNC9QKWjKP+qi0=";
  };

  nativeBuildInputs = [ setuptools ];
}
