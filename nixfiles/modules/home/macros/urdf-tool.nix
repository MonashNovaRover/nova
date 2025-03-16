{ pkgs ? import <nixpkgs> { } }:

let
  # Define onshape-to-robot build
  onshapeToRobot = pkgs.python3Packages.buildPythonApplication rec {
    pname = "onshape-to-robot";
    version = "1.2.1";

    src = pkgs.fetchFromGitHub {
      owner = "V01DBREAKER";
      repo = pname;
      rev = "03419bd1a20f901ff6325e467e07d43b502436bb";
      hash = "sha256-/KguWFqhmLyqnDDLX96RFxrSeLvb9Wjkpbhy+7IZlbs=";
    };

    buildInputs = with pkgs.python3Packages; [
      numpy
      pybullet
      requests
      commentjson
      colorama
      numpy-stl
      transforms3d
      pymeshlab
      python-dotenv
    ];

    nativeBuildInputs = [
      pkgs.pkg-config
    ];

    postPatch = ''
      substituteInPlace onshape_to_robot/edit_shape.py \
        --replace "os.system('cd '+directory+'; openscad '+os.path.basename(fileName)+')'" \
        'subprocess.run(["openscad", os.path.basename(fileName)], cwd=directory, check=True)'
    '';

    meta = with pkgs.lib; {
      description = "A Python library to convert Onshape assemblies into URDF or SDF robots";
      homepage = "https://github.com/Rhoban/onshape-to-robot";
      license = licenses.mit;
    };
  };

  # Check if the Onshape API keys file exists before importing it
  onshapeKeys = if builtins.pathExists "/etc/nixos/onshape_key/onshape-keys.nix" then
    import /etc/nixos/onshape_key/onshape-keys.nix
  else
    {
      ACCESS_KEY = "";
      SECRET_KEY = "";
    };  # If the file does not exist, use set with empty values to avoid errors

  urdf-modifier = pkgs.python3Packages.buildPythonApplication rec {
    pname = "urdf-modifier";
    version = "1.0";

    src = pkgs.fetchFromGitHub {
      owner = "V01DBREAKER";
      repo = pname;
      rev = "eac72564eadf6778ea63e4d89b3cd588ecb3c316";
      hash = "sha256-OtwYxlHKxruVxJmosMbvT//FM5gPV+75MCG1zKyEuuE=";
    };

    buildInputs = with pkgs.python3Packages; [
      pymeshlab
    ];

    meta = with pkgs.lib; {
      description = "A Python script used to modify generated URDFs and recalculate inertias.";
      homepage = "https://github.com/V01DBREAKER/urdf-modifier";
      license = licenses.mit;
    };
  };

in
pkgs.mkShell {
  buildInputs = [
    pkgs.openscad
    onshapeToRobot
    urdf-modifier
    pkgs.python3Packages.numpy
    pkgs.python3Packages.pybullet
    pkgs.python3Packages.requests
    pkgs.python3Packages.commentjson
    pkgs.python3Packages.colorama
    pkgs.python3Packages.numpy-stl
    pkgs.python3Packages.transforms3d
    pkgs.python3Packages.pymeshlab
    pkgs.python3Packages.python-dotenv
  ];

  # Set up PYTHONPATH to include all dependencies
  shellHook = ''
    export PATH=${onshapeToRobot}/bin:$PATH
    export PATH=${urdf-modifier}/bin:$PATH
    export PYTHONPATH=${pkgs.python3Packages.python.sitePackages}:${pkgs.python3Packages."numpy-stl"}/lib/python3.12/site-packages:$PYTHONPATH
    export ONSHAPE_API=https://cad.onshape.com
    # Check if Onshape API keys are defined, and warn if not
    if [ -z "${onshapeKeys.ACCESS_KEY}" ] || [ -z "${onshapeKeys.SECRET_KEY}" ]; then
      echo -e "\033[1;31mWARNING: Onshape API keys (ACCESS_KEY, SECRET_KEY) are not defined in /etc/nixos/onshapeAPI/onshape-keys.nix. Please configure them before using the Onshape API.\033[0m"
    else
      export ONSHAPE_ACCESS_KEY=${onshapeKeys.ACCESS_KEY}
      export ONSHAPE_SECRET_KEY=${onshapeKeys.SECRET_KEY}
    fi

    echo "OpenSCAD and Onshape-to-Robot are ready to use."
  '';
}