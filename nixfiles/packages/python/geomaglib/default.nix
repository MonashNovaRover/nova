{ buildPythonPackage
, fetchurl
, setuptools
, numpy
}:

buildPythonPackage rec {
  pname = "geomaglib";
  version = "1.2.3";
  pyproject = true;

  src = fetchurl {
    url = "https://files.pythonhosted.org/packages/ed/cf/815c4ead29d70670ae35097b3cc5e8d2a7d7bad95619c28e0092e283c59b/geomaglib-1.2.3.tar.gz";
    hash = "sha256-cdNFiE/lghmQDjr38GLDAgI6MdMlQyYEyx0OpLHjA6o=";
  };

  build-system = [ setuptools ];

  propagatedBuildInputs = [
    numpy
  ];

  pythonImportsCheck = [ "geomaglib" ];
}
