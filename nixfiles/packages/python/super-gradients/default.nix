{ buildPythonPackage
, fetchFromGitHub
, python3Packages
, callPackage
}:

let
  data-gradients = callPackage ./data-gradients.nix { };
  #onnx-simplifier = callPackage ./onnx-simplifier.nix { };
in
buildPythonPackage rec {
  pname = "super-gradients";
  version = "8.0.146";

  src = fetchFromGitHub {
    owner = "Deci-AI";
    repo = pname;

    rev = "d7152a4d3b92f1be339f71493135627f9a3529c8";
    hash = "sha256-51TWJatypEkTnh+0VsQSt9UFHIh0f7Lp/bKhnyjijeE=";
  };

  propagatedBuildInputs = with python3Packages; [
    pytorch
    tqdm
    boto3
    jsonschema
    deprecated
    scipy
    matplotlib
    psutil
    tensorboard
    setuptools
    torchvision
    torchmetrics
    hydra-core
    onnxruntime
    onnx
    pillow
    pip-tools
    einops
    treelib
    termcolor
    packaging
    wheel
    pygments
    stringcase
    rapidfuzz
    json-tricks
    #onnx-simplifier i hate this dependency chain so i deleted it lol
    data-gradients
    albumentations
    fonttools
    werkzeug
    imagesize
  ];
}
