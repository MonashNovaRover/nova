{ lib
, buildPythonPackage
, jcan
, pyside6
}:

buildPythonPackage {
  name = "can_sleuth";

  src = builtins.path rec {
    name = "can_sleuth-source";
    path = ../../..;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [ jcan pyside6 ];
}
