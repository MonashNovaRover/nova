{ buildPythonPackage
, fetchFromGitHub
, coverage
, fonttools
, hydra-core
, jinja2
, matplotlib
, numpy
, omegaconf
, opencv4
, pillow
, platformdirs
, pygments
, pytorch
, rapidfuzz
, scipy
, seaborn
, tensorboard
, tqdm
, torchvision
, werkzeug
, wheel
}:

buildPythonPackage rec {
  pname = "data-gradients";
  version = "0.3.2";

  src = fetchFromGitHub {
    owner = "Deci-AI";
    repo = pname;

    rev = "2045e25cd2cf385b26b87dd22506aa2388b97ffa";
    hash = "sha256-B2IuNMTZnzBi6IxrHBoMDsmIcqGQpznd/2f1XKo1Oa4=";
  };

  propagatedBuildInputs = [
    coverage
    fonttools
    hydra-core
    # imagededup duplicate error
    jinja2
    matplotlib
    numpy
    omegaconf
    opencv4
    pillow
    platformdirs
    pygments
    pytorch
    rapidfuzz
    scipy
    seaborn
    tensorboard
    tqdm
    torchvision
    werkzeug
    wheel
    # xhtml2pdf pyhanko error https://github.com/NixOS/nixpkgs/issues/355162
  ];

  postPatch = ''
      # The package name is just "opencv", not "opencv-python".
      # https://discourse.nixos.org/t/how-to-give-opencv-dependency-to-python-package/16949
      sed -i 's/opencv-python/opencv/g' requirements.txt
    '';
}
