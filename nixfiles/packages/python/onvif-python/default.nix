{ lib
, pkgs
, buildPythonPackage
, fetchPypi
, fetchurl
, pythonOlder
, requests
, setuptools
, zeep
}:

let
  # onvif-python needs wheel >= 0.48.0, we had 0.47.0
  customPython = pkgs.python3.override {
    packageOverrides = pyFinal: pyPrev: {
      wheel = pyPrev.wheel.overridePythonAttrs (old: rec {
        version = "0.48.0";
        src = pyPrev.fetchPypi {
          pname = "wheel";
          inherit version;
          hash = "sha256-lIAHZWAekXG/XVjQZuZAZihCvO3Lq5grLJB4eiyYcyI=";
        };
      });
    };
  };
in
  customPython.pkgs.buildPythonPackage rec {
    pname = "onvif-python";
    version = "0.4.7";
    pyproject = true;

    disabled = pythonOlder "3.10";

    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/de/19/4458bf306428ed287f877dd6b500c189b90a352a5eb6a395e4f10c9ad2eb/onvif_python-0.4.7.tar.gz";
      hash = "sha256-5LAkoYXTRJzSwu3qXC94ymbbLVWdtZKLavrQ/ZzQGgE=";
    };

    build-system = with customPython.pkgs; [
      setuptools
      wheel
    ];

    propagatedBuildInputs = [
      zeep
      requests
    ];

    pythonImportsCheck = [ "onvif" ];
  }