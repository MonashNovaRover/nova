{ lib
, buildPythonPackage
, mock-jcan
, pytest
}:

buildPythonPackage {
  name = "nova-pytest-framework";

  src = builtins.path rec {
    name = "nova-pytest-framework-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    mock-jcan
    pytest
  ];
}
