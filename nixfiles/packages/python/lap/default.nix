{ buildPythonPackage
, fetchFromGitHub
, numpy
, setuptools
, python3Packages
}:

  buildPythonPackage rec {
    pname = "lap";
    version = "0.5.12";

    src = fetchFromGitHub {
      owner = "gatagat";
      repo = pname;
      rev = "600c210d9bef793ee0fe502cbc350e676a6e083a";
      hash = "sha256-ktLwdeb7UWhdihOhdeYIi6Geyp7aJsVPPec22MtI9Jo=";
    };
    nativeBuildInputs = [python3Packages.cython];

    propagatedBuildInputs = [
      numpy
      setuptools
    ];
  }
