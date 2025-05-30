{ lib
, buildPythonPackage
, fetchFromGitHub
, flit-core
, pyserial
}:

buildPythonPackage rec {
  pname = "minimalmodbus";
  version = "2.1.1"; # Replace with the actual version you want

  src = fetchFromGitHub {
    owner = "pyhys";
    repo = "minimalmodbus";
    rev = "${version}";
    hash = "sha256-cHTUQhLUgLOiV0Mj7uBfHZ0Wi47Np+Uula8Q3xDii30="; # Replace with actual hash
  };

  format = "pyproject";

  nativeBuildInputs = [ flit-core ];
  propagatedBuildInputs = [ pyserial ];

  pythonImportsCheck = [ "minimalmodbus" ];

  meta = with lib; {
    description = "Easy-to-use Modbus RTU and Modbus ASCII implementation for Python";
    homepage = "https://github.com/pyhys/minimalmodbus";
    license = licenses.asl20;
    maintainers = with maintainers; [ ];
    platforms = platforms.all;
  };
}