{ buildPythonPackage
, fetchFromGitHub
, python3Packages 
, cmake
}:

# why is this not in nikpkgs man ahhhhhhhhhhh im in dependancy hell (theres probably a better way to do this but im 3 packages deep already)
# this was separated from onnx in v1.9.0 :skull:

buildPythonPackage rec {
  pname = "onnxoptimizer";
  version = "0.3.19";

  src = fetchFromGitHub {
    owner = "onnx";
    repo = "optimizer";

    rev = "b3a4611861734e0731bbcc2bed1f080139e4988b";
    hash = "sha256-PIm039A/YccE5dRMXt1lptQ+kxxdQFtAdkZZQaiGo9c=";
  };

  nativeBuildInputs = [cmake];

  propagatedBuildInputs = with python3Packages; [
    onnx
    protobuf
  ];
}
