{ buildPythonPackage
, fetchFromGitHub
, matplotlib
, numpy
, opencv4
, pillow
, pyyaml
, requests
, scipy
, torch
, torchvision
, tqdm
, pandas
, seaborn
, psutil
, py-cpuinfo
}:

buildPythonPackage rec {
  pname = "ultralytics";
  version = "8.0.146";

  src = fetchFromGitHub {
    owner = pname;
    repo = pname;

    # There are no version tags.
    # https://github.com/ultralytics/ultralytics/issues/4039
    rev = "c3c27b019a9516a9b2c78c291b61ef7cf97ff7f3";
    hash = "sha256-QX+ly2UDye4b10HaFXf7XxJGWDxd30Vq6ecBtNI7BHE=";
  };

  propagatedBuildInputs = [
    # Base
    matplotlib
    numpy
    opencv4
    pillow
    pyyaml
    requests
    scipy
    torch
    torchvision
    tqdm

    # Plotting
    pandas
    seaborn

    # Extras
    psutil
    py-cpuinfo
  ];

  postPatch = ''
    # The package name is just "opencv", not "opencv-python".
    # https://discourse.nixos.org/t/how-to-give-opencv-dependency-to-python-package/16949
    sed -i 's/opencv-python/opencv/g' requirements.txt
  '';

  # https://github.com/ultralytics/ultralytics/issues/3961
  doCheck = false;
}
