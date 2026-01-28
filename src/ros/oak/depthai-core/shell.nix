let
  pkgs = import (fetchTarball "https://github.com/Redecorating/nixpkgs/archive/b1e2040354fbfba3eb96e71296a07b58157f829e.tar.gz") {};
in pkgs.mkShell {
  packages = [
    pkgs.depthai-core
    (pkgs.python3.withPackages (python-pkgs: with python-pkgs; [
      numpy
      pillow
      opencv4
    ]))
  ];
}

