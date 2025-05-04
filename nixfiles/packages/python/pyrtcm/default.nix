{
  buildPythonPackage,
  fetchurl,
  pytestCheckHook,
  pytest-cov-stub,
  pythonOlder,
  setuptools,
}:

buildPythonPackage rec {
  pname = "pyrtcm";
  version = "1.1.5";
  pyproject = true;

  disabled = pythonOlder "3.9";

  src = fetchurl {
    url = "https://github.com/semuconsulting/pyrtcm/archive/refs/tags/v1.1.5.tar.gz";
    name = "v1.1.5.tar.gz";
    sha256 = "sha256-oG6DQ0NBuaBqZA5/7b557Od1591FmPso8CIlLZm7QBc=";
  };

  build-system = [ setuptools ];

  nativeCheckInputs = [
    pytestCheckHook
    pytest-cov-stub
  ];

  pythonImportsCheck = [ "pyrtcm" ];
}