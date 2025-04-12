{ buildPythonPackage
, fetchPypi
}:

buildPythonPackage rec {
  pname = "pyqt5-stubs";
  version = "5.15.6.0";
  src = fetchPypi {
    pname = "PyQt5-stubs";
    inherit version;
    hash = "sha256-kScKwj6/OKHcBM2XqoUs0Ir4Lcg5EA5Tla8UR+Pplwc=";
  };
}
