{ buildPythonPackage
, fetchFromGitHub
, python3Packages 
, callPackage
, cmake
}:

let
  onnxoptimizer = callPackage ./onnxoptimizer.nix { };
in
buildPythonPackage rec {
  pname = "onnx-simplifier";
  version = "0.4.36";

  src = fetchFromGitHub {
    owner = "daquexian";
    repo = pname;

    rev = "fbf1ca8e26ba29200f6572194391b148c0695254";
    hash = "sha256-rN7fA46Jd0l1oQH2mb37SxtEWDq+yqhcxQfX7eDUog0=";
  };

  nativeBuildInputs = [cmake];

  propagatedBuildInputs = with python3Packages; [
    onnx
    onnxoptimizer
    onnxruntime
    protobuf
    rich
  ];
}
