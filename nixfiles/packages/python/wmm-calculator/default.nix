{ buildPythonPackage
, fetchurl
, pythonOlder
, setuptools
, python3Packages
, geomaglib
}:

buildPythonPackage rec {
  pname = "wmm-calculator";
  version = "1.4.3";
  pyproject = true;

  disabled = pythonOlder "3.9";

  src = fetchurl {
    url = "https://files.pythonhosted.org/packages/94/99/39f129c7260071617a19a29c6a04589180a05d84a76ae830377c3c6514a1/wmm_calculator-1.4.3.tar.gz";
    hash = "sha256-Hvh5nsKfZmt6ZuiLDdNspNKmOvwrJ6tr0ohDbOh3Wlc=";
  };

  build-system = [ setuptools ];

  propagatedBuildInputs = with python3Packages; [
    numpy
    geomaglib
  ];

  pythonImportsCheck = [ "wmm" ];
}
