{ 
  bleach, 
  bleach-allowlist, 
  buildPythonPackage, 
  fetchFromGitHub, 
  mkdocs, 
  pytest-cov, 
  pytestCheckHook, 
  setuptools, 
}:

buildPythonPackage rec {
  pname = "mkdocs-safe-text-plugin";
  version = "1.6.1";

  pyproject = true;

  src = fetchFromGitHub {
    owner = "raimon49";
    repo = pname;
    rev = "v-${version}";
    hash = "sha256-gH6JVaRIQUIl4AXEka6tfHcwLQWlL0zOjZBijDU5Z10=";
  };

  nativeBuildInputs = [ 
    setuptools 
  ];

  propagatedBuildInputs = [
    bleach
    bleach-allowlist
    mkdocs
  ];

  nativeCheckInputs = [
    pytest-cov
    pytestCheckHook
  ];

  # Requires pytest-pycodestyle
  doCheck = false;
}
