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

  src = fetchFromGitHub {
    owner = "raimon49";
    repo = pname;
    rev = "v-${version}";
    hash = "sha256-RVGaHhgIdUTW4rBXPbdp7tMeeYRYPmqRoRuvSCJmCiU=";
  };

  nativeBuildInputs = [
    pytest-runner
  ];

  propagatedBuildInputs = [
    mkdocs
    bleach
    bleach-allowlist
  ];

  nativeCheckInputs = [
    pytest
    pytest-runner
    pytest-cov
  ];

  # Requires pytest-pycodestyle
  doCheck = false;
}
