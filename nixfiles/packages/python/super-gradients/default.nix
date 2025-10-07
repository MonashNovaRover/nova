{ buildPythonPackage
, fetchFromGitHub
, callPackage
, albumentations
, boto3
, deprecated
, einops
, fonttools
, hydra-core
, imagesize
, json-tricks
, jsonschema
, matplotlib
, onnx
, onnxruntime
, packaging
, pillow
, pip-tools
, psutil
, pygments
, pytorch
, rapidfuzz
, scipy
, setuptools
, stringcase
, tensorboard
, termcolor
, torchmetrics
, torchvision
, treelib
, tqdm
, werkzeug
, wheel
}:

let
  data-gradients = callPackage ./data-gradients.nix { };
  # onnx-simplifier = callPackage ./onnx-simplifier.nix { };
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

  propagatedBuildInputs = [
    albumentations
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
    # onnx-simplifier i hate this dependency chain so i deleted it lol
    onnxruntime
    packaging
    pillow
    pip-tools
    psutil
    pygments
    pytorch
    rapidfuzz
    scipy
    setuptools
    stringcase
    tensorboard
    termcolor
    torchmetrics
    torchvision
    treelib
    tqdm
    werkzeug
    wheel
  ];
}
