{ buildPythonPackage, 
  fetchgit, 
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

  src = fetchgit {
    url = "https://github.com/MonashNovaRover/pynmeagps";
    rev = "227293afe8d376c7af55dde1ae76fc279c4e838d";
    hash = "sha256-ytyFXGDs45BSc4liLjCZsbYrvYIA1OEtMbeVhzM5wik=";
  };

  build-system = [ setuptools ];

  nativeCheckInputs = [
    pytestCheckHook
    pytest-cov-stub
  ];

  pythonImportsCheck = [ "pynmeagps" ];
}