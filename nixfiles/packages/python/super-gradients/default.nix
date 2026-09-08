{ 
  albumentationsx, 
  boto3, 
  buildPythonPackage, 
  callPackage, 
  deprecated, 
  einops, 
  fetchFromGitHub, 
  fonttools, 
  hydra-core, 
  imagesize, 
  json-tricks, 
  jsonschema, 
  matplotlib, 
  onnx, 
  onnxruntime, 
  packaging, 
  pillow, 
  pip-tools, 
  psutil, 
  pygments, 
  rapidfuzz, 
  scipy, 
  setuptools, 
  sphinx, 
  sphinx-rtd-theme, 
  stringcase, 
  tensorboard, 
  termcolor, 
  torch, 
  torchmetrics, 
  torchvision, 
  tqdm, 
  treelib, 
  werkzeug, 
  wheel, 
}:

let
  data-gradients = callPackage ./data-gradients.nix { };
in
buildPythonPackage rec {
  pname = "super-gradients";
  version = "3.7.1";
  pyproject = true;

  src = fetchFromGitHub {
    owner = "Deci-AI";
    repo = pname;
    rev = version;
    hash = "sha256-51TWJatypEkTnh+0VsQSt9UFHIh0f7Lp/bKhnyjijeE=";
  };

  buildInputs = [
    albumentationsx
    boto3
    data-gradients
    deprecated
    einops
    fonttools
    hydra-core
    imagesize
    json-tricks
    jsonschema
    matplotlib
    onnx
    onnxruntime
    packaging
    pillow
    pip-tools
    psutil
    pygments
    rapidfuzz
    scipy
    setuptools
    sphinx
    sphinx-rtd-theme
    stringcase
    tensorboard
    termcolor
    torch
    torchmetrics
    torchvision
    tqdm
    treelib
    werkzeug
    wheel
  ];

  patches = [
    ../../../overlay/ros/patches/super-gradients.patch
  ];

  # This derivation has not been maintained; significant patching needs to occur to the source files.
  # This is a temporary fix.
  # To resolve this issue properly, there are 3 (or more) options:
  # 1. Convert this to a flake and use an earlier version of nixpkgs as an input
  # 2. Manually override each dependency to an earlier version (this isn't hard, just tedious)
  # 3. Open a PR against upstream source files with new versions and fixes, then patch the PR into the source files above.
  dontCheckRuntimeDeps = true;
}