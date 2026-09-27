{ 
  buildPythonPackage, 
  coverage, 
  cython, 
  fetchFromGitHub, 
  fonttools, 
  hydra-core, 
  imagededup, 
  jinja2, 
  matplotlib, 
  numpy, 
  omegaconf, 
  opencv4, 
  pillow, 
  platformdirs, 
  pygments, 
  pywavelets, 
  rapidfuzz, 
  scikit-learn, 
  scipy, 
  seaborn, 
  setuptools, 
  tensorboard, 
  torch, 
  torchvision, 
  tqdm, 
  werkzeug, 
  wheel, 
}:

buildPythonPackage rec {
  pname = "data-gradients";
  version = "0.3.2";
  pyproject = false;

  src = fetchFromGitHub {
    owner = "Deci-AI";
    repo = pname;
    rev = version;
    hash = "sha256-B2IuNMTZnzBi6IxrHBoMDsmIcqGQpznd/2f1XKo1Oa4=";
  };

  nativeBuildInputs = [
    setuptools
  ];

  propagatedBuildInputs = [
    coverage
    cython
    fonttools
    hydra-core
    imagededup
    jinja2
    matplotlib
    numpy
    omegaconf
    opencv4
    pillow
    platformdirs
    pygments
    pywavelets
    rapidfuzz
    scikit-learn
    scipy
    seaborn
    tensorboard
    torch
    torchvision
    tqdm
    werkzeug
    wheel
  ];
}