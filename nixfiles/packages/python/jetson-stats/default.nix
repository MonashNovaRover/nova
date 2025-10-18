{ buildPythonPackage
, fetchPypi
, nvidia-ml-py
, distro
, smbus2
}:

buildPythonPackage rec {
  pname = "jetson-stats";
  version = "4.3.2";

  src = fetchPypi {
    inherit pname version;
    sha256 = "";
  };

  propagatedBuildInputs = [
    nvidia-ml-py
    distro
    smbus2
  ];
}
