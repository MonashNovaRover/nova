{ lib
, buildPythonPackage
#, rclpy
, mock-jcan
, pytest
}:

buildPythonPackage {
  name = "nova-pytest-framework";
  format = "setuptools";

  src = builtins.path rec {
    name = "nova-pytest-framework-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedNativeBuildInputs = [
    mock-jcan
  ];

  propagatedBuildInputs = [
    pytest
  ];

  BuildInputs = [
    # rclpy
  ];

  setupHook = ./setup-hook.sh;
}
