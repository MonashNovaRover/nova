{ buildPythonPackage
, fetchFromGitHub
, mkdocs
, bleach
, bleach-allowlist
, pytest
, pytest-runner
, pytest-cov
}:

buildPythonPackage rec {
  pname = "mkdocs-safe-text-plugin";
  version = "1.5.1";

  pyproject = true;

  src = fetchFromGitHub {
    owner = "raimon49";
    repo = pname;
    rev = "v-${version}";
    hash = "sha256-RVGaHhgIdUTW4rBXPbdp7tMeeYRYPmqRoRuvSCJmCiU=";
  };

  nativeBuildInputs = [
    pytestrunner
  ];

  propagatedBuildInputs = [
    mkdocs
    bleach
    bleach-allowlist
  ];

  nativeCheckInputs = [
    pytest
    pytestrunner
    pytest-cov
  ];

  # Requires pytest-pycodestyle
  doCheck = false;
}
