let
  pkgs = import <nixpkgs> {};
in pkgs.mkShell {
  packages = [
    (pkgs.python3.withPackages (python-pkgs: [
      python-pkgs.torch
      python-pkgs.torchvision
      python-pkgs.numpy
    ]))
  ];
  shellHook = ''
    alias predict="python3 ~/nova/src/other/ilmenite_ml/predict_ilmenite.py"
    alias predict-cpu="python3 ~/nova/src/other/ilmenite_ml/predict_ilmenite_cpu.py"
  '';
}

#
# python312Packages.torchvision
# python312Packages.numpy

