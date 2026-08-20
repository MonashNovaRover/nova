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
  , pywavelets
  , scikit-learn
  , torch
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
  pname = "imagededup";
  version = "0.3.2";

  pyproject = true;

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
    scikit-learn
    torch
    pywavelets
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
      sed -i 's/opencv-python/opencv/g' requirements.txt 2>/dev/null || true
      # imagededup's Cython extensions are incompatible with Python 3.14
      # (_PyLong_AsByteArray signature changed). Remove it from dependencies entirely.
      sed -i '/imagededup/d' requirements.txt 2>/dev/null || true
      # xhtml2pdf causes pyhanko errors https://github.com/NixOS/nixpkgs/issues/355162
      sed -i '/xhtml2pdf/d' requirements.txt 2>/dev/null || true
    '';
}
