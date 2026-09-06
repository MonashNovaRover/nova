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

# # Pre-existing pynmeagps implementation that may be useful.
#   pynmeagps = pySuper.pynmeagps.overridePythonAttrs ({ prePatch ? "", ... }: {
#   src = self.fetchFromGitHub {
#     owner = "MonashNovaRover";
#     repo = "pynmeagps";
#     rev = "50e93bddeae0a2957363e18ee44a032307881675";
#     hash = "sha256-SGTC/W7wv/we8Lo07geEA8h/PcsmG7BIzGHfgL3h4ZA=";
#   };

#   prePatch = prePatch + ''
#     substituteInPlace pyproject.toml \
#       --replace-warn '--cov --cov-report term-missing --cov-fail-under 95' '--cov --cov-report html --cov-fail-under 98'
#   '';
# });
}