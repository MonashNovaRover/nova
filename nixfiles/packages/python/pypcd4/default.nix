{ buildPythonPackage
, fetchurl
, numpy
, pydantic
, python-lzf
}:

buildPythonPackage rec {
  pname = "pypcd4";
  version = "1.3.0";
  format = "wheel";

  src = fetchurl {
    url = "https://files.pythonhosted.org/packages/bc/ff/5798fb078a3ed196940b8cc2547297eaba82c808bbeb03c59f82f577590b/pypcd4-1.3.0-py3-none-any.whl";
    hash = "sha256-M1YwPJjBcCK7ZF0dLoCmdFxV0BnEsOrceWhCjIWDyWE=";
  };

  propagatedBuildInputs = [
    numpy
    pydantic
    python-lzf
  ];

  pythonImportsCheck = [ "pypcd4" ];
}
