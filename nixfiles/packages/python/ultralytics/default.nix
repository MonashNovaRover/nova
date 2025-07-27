{ buildPythonPackage
, fetchFromGitHub
, callPackage
, matplotlib
, numpy
, opencv4
, pandas
, pillow
, pip
, psutil
, py-cpuinfo
, pyyaml
, requests
, seaborn
, setuptools
, scipy
, torch
, torchvision
, tqdm
}:

let
  ultralytics-thop = callPackage ./ultralytics-thop.nix {};
in
  buildPythonPackage rec {
    pname = "ultralytics";
    version = "8.3.71";
    pyproject = true;

    src = fetchFromGitHub {
      owner = pname;
      repo = pname;
      rev = "5bca9341e8da3f5c99cb9edbb747fda7ddfe78fb";
      hash = "sha256-KczxqflfyDl1i8I/Zn7PxkzAqITbflydZAhzP5p4DpI=";
    };

    propagatedBuildInputs = [
      matplotlib
      numpy
      opencv4
      pandas
      pillow
      psutil
      py-cpuinfo
      pyyaml
      requests
      seaborn
      scipy
      torch
      torchvision
      tqdm
      ultralytics-thop
    ];

    nativeBuildInputs = [
      pip
      setuptools
    ];

    # lock torchvision version
    # https://github.com/NixOS/nixpkgs/issues/308154
    # The package name is just "opencv", not "opencv-python".
    # https://discourse.nixos.org/t/how-to-give-opencv-dependency-to-python-package/16949
    preBuild = ''
      sed -i '/torchvision>=0.9.0/d' pyproject.toml
      sed -i 's/opencv-python/opencv/g' pyproject.toml
    '';

    # https://github.com/ultralytics/ultralytics/issues/3961
    doCheck = false;
  }
