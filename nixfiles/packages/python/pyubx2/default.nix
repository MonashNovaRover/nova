{
  buildPythonPackage, 
  fetchurl, 
  pytestCheckHook, 
  pytest-cov-stub, 
  pythonOlder, 
  setuptools, 
  python3Packages, 
  pynmeagps, 
  pyrtcm, 
}:

buildPythonPackage rec {
  pname = "pyubx2";
  version = "1.2.51";
  pyproject = true;

  disabled = pythonOlder "3.9";

  src = fetchurl {
    url = "https://github.com/semuconsulting/pyubx2/archive/refs/tags/v1.2.51.tar.gz";
    name = "v1.2.51.tar.gz";
    sha256 = "sha256-yOSsDe1+DOg7BJKeOnO6V7x+nOiEG+kv+gNIbQpF3iE=";
  };

  build-system = [ setuptools ];

  nativeCheckInputs = [
    pytestCheckHook
    pytest-cov-stub
  ];

  propagatedBuildInputs = with python3Packages; [
    pynmeagps
    pyrtcm
  ];

  pythonImportsCheck = [ "pyubx2" ];
}