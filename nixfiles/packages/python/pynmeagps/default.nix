{
  buildPythonPackage, 
  fetchurl, 
  pytestCheckHook, 
  pytest-cov-stub, 
  pythonOlder, 
  setuptools, 
}:

buildPythonPackage rec {
  pname = "pynmeagps";
  version = "1.0.49";
  pyproject = true;

  disabled = pythonOlder "3.9";

  src = fetchurl {
    url = "https://github.com/semuconsulting/pynmeagps/archive/refs/tags/v1.0.49.tar.gz";
    name = "v1.0.49.tar.gz";
    sha256 = "sha256-sT3JhMjkUJsvyQ5D3dWW0ysb7eNnZjE2ZIps2eKczWU=";
  };

  build-system = [ setuptools ];

  nativeCheckInputs = [
    pytestCheckHook
    pytest-cov-stub
  ];

  pythonImportsCheck = [ "pynmeagps" ];
}