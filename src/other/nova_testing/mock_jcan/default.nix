{ lib
, buildPythonPackage
}:

buildPythonPackage {
  name = "mock-jcan";

  src = builtins.path rec {
    name = "mock-jcan-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  setupHook = ./setup-hook.sh;
}
