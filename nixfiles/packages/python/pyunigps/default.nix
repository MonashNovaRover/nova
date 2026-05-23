{
  buildPythonPackage,
  fetchurl,
  pytestCheckHook,
  pytest-cov-stub,
  pythonOlder,
  setuptools,
  pynmeagps,
  pyrtcm,
}:

buildPythonPackage {
  pname = "pyunigps";
  version = "1.0.0";
  pyproject = true;

  disabled = pythonOlder "3.9";

  src = fetchurl {
    url = "https://github.com/semuconsulting/pyunigps/archive/refs/tags/v1.0.0.tar.gz";
    name = "v1.0.0.tar.gz";
    sha256 = "sha256-kcmiDfV7FvMqxgjixKeqCHJov7AOVoQhDdsQvTa7biw=";
  };

  build-system = [ setuptools ];

  nativeCheckInputs = [
    pytestCheckHook
    pytest-cov-stub
  ];

  propagatedBuildInputs = [
    pynmeagps
    pyrtcm
  ];

  pythonImportsCheck = [ "pyunigps" ];
}
