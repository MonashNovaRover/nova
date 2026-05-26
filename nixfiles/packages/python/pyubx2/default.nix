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

buildPythonPackage rec {
  pname = "pyubx2";
  version = "1.2.60";
  pyproject = true;

  disabled = pythonOlder "3.10";

  src = fetchurl {
    url = "https://github.com/semuconsulting/pyubx2/archive/refs/tags/v${version}.tar.gz";
    # Let Nix fail once to give you the correct hash, or run:
    # nix-prefetch-url --unpack https://github.com/semuconsulting/pyubx2/archive/refs/tags/v1.2.60.tar.gz
    sha256 = "sha256-x+w1wpk5S4YdFq79ue8tTq25UeMzNsZzjgtYSAHPMxk=";
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

  pythonImportsCheck = [ "pyubx2" ];
}