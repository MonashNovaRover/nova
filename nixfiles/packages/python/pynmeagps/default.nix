{ buildPythonPackage, 
  fetchurl,
  pytestCheckHook, 
  pytest-cov-stub, 
  pythonOlder, 
  setuptools, 
}:

buildPythonPackage {
  pname = "pynmeagps";
  version = "1.1.4";
  pyproject = true;

  disabled = pythonOlder "3.9";

  src = fetchurl {
    url = "https://github.com/semuconsulting/pynmeagps/archive/refs/tags/v1.1.4.tar.gz";
    name = "v1.1.4.tar.gz";
    sha256 = "sha256-xqRsWxal4FV6isiMDd9SoFfQU1pklUu9bU3ip6+mbNk=";
  };

  build-system = [ setuptools ];

  nativeCheckInputs = [
    pytestCheckHook
    pytest-cov-stub
  ];

  pythonImportsCheck = [ "pynmeagps" ];
}