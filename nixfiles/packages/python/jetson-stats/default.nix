{ buildPythonPackage
, fetchPypi
, nvidia-ml-py
, distro
, smbus2
, setuptools
}:

buildPythonPackage rec {
  pname = "jetson-stats";
  version = "4.3.2";

  src = fetchPypi {
    inherit pname version;
    sha256 = "sha256-KwM8sCudTPcNE825FbXPWLhkp0LWEnBFE+GWlD3Ao5c=";
  };

  nativeBuildInputs = [
    setuptools
  ];
  format = "setuptools";

  postPatch = ''
    # Make is_superuser() always return True to bypass root check
    sed -i 's/return os.getuid() == 0/return True/' setup.py

    # Replace the function body to always return True
    sed -i '/def is_virtualenv()/,/^$/ s/return .*$/return True/' $(grep -rl "def is_virtualenv" .)
  '';

  propagatedBuildInputs = [
    nvidia-ml-py
    distro
    smbus2
  ];
}
