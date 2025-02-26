{ pkgs }:
  pkgs.python3Packages.buildPythonPackage rec {
  pname = "ultralytics_thop";
  version = "2.0.13";
  src = pkgs.python3Packages.fetchPypi {
    inherit pname version;
    sha256 = "afd71619b61aa1d11858d69fff29c1c1e438a1c76bc274e0eaa040548627c384";
  };
  pyproject = true;
  propagatedBuildInputs = with pkgs.python3Packages; [
    numpy
    setuptools
    torch
  ];
  nativeBuildInputs = with pkgs.python3Packages; [
    setuptools 
    pip 
  ];
  meta = {
    description = "Ultralytics";
    homepage = "https://github.com/ultralytics/ultralytics-thop";
    license = pkgs.lib.licenses.mit;
    platforms = pkgs.lib.platforms.all;
  };
}
