{ pkgs ? import <nixpkgs> { } }:

let
  python = pkgs.python312Full;
  py = python.pkgs;

  # Rebuild pymeshlab in your environment to ensure same numpy
  pyEnv = pkgs.python312.override {
    packageOverrides = self: super: {
      pymeshlab = super.pymeshlab.overrideAttrs (old: {
        propagatedBuildInputs = (old.propagatedBuildInputs or []) ++ [ self.numpy ];
      });
    };
  };
  pyPkgs = pyEnv.pkgs;

  onshapeToRobot = pyPkgs.buildPythonPackage rec {
    pname = "onshape-to-robot";
    version = "1.5.7";

    src = pkgs.fetchFromGitHub {
      owner = "Rhoban";
      repo = pname;
      rev = "1c6e662fe6233b452dfc0de1632a40086ba1b8f7";
      hash = "sha256-f/ho5NSNm7nfJVsF6xczobc9puny3/EtS/Idsl6Sjus=";
    };

    propagatedBuildInputs = with pyPkgs; [
      numpy
      requests
      commentjson
      colorama
      numpy-stl
      transforms3d
      python-dotenv
      pymeshlab
    ];

    nativeBuildInputs = [ pkgs.pkg-config ];

    postPatch = ''
      substituteInPlace onshape_to_robot/edit_shape.py \
        --replace "os.system('cd '+directory+'; openscad '+os.path.basename(fileName)+')'" \
        'subprocess.run(["openscad", os.path.basename(fileName)], cwd=directory, check=True)'
    '';

    meta = with pkgs.lib; {
      description = "Convert Onshape assemblies into URDF or SDF robots";
      homepage = "https://github.com/Rhoban/onshape-to-robot";
      license = licenses.mit;
    };
  };

  urdfModifier = pyPkgs.buildPythonApplication rec {
    pname = "urdf-inertia-script";
    version = "1.1";

    src = pkgs.fetchFromGitHub {
      owner = "V01DBREAKER";
      repo = pname;
      rev = "ed09f00c6257cec36d8c3ed30fb50905cbcbb872";
      hash = "sha256-dNhATy4ZtYQoj44O6VwtbB8k0hM3bSAp8W+nOFx9ik8=";
    };

    propagatedBuildInputs = [ pyPkgs.pymeshlab ];

    meta = with pkgs.lib; {
      description = "Modify generated URDFs and recalculate inertias.";
      homepage = "https://github.com/V01DBREAKER/urdf-inertia-script";
      license = licenses.mit;
    };
  };

  onshapeKeys = if builtins.pathExists "/etc/nixos/onshape_key/onshape-keys.nix" then
    import /etc/nixos/onshape_key/onshape-keys.nix
  else
    {
      ACCESS_KEY = "";
      SECRET_KEY = "";
    };

in
pkgs.mkShell {
  packages = [
    pkgs.openscad
    onshapeToRobot
    urdfModifier
    pyPkgs.pymeshlab
  ];

  shellHook = ''
    export PATH=${onshapeToRobot}/bin:${urdfModifier}/bin:$PATH
    export PYTHONPATH=${onshapeToRobot}/${python.sitePackages}:$PYTHONPATH
    export ONSHAPE_API=https://cad.onshape.com

    ACCESS_KEY="${onshapeKeys.ACCESS_KEY}"
    SECRET_KEY="${onshapeKeys.SECRET_KEY}"

    if [ -z "$ACCESS_KEY" ] || [ -z "$SECRET_KEY" ]; then
      echo -e "\033[1;31mWARNING: Onshape API keys not defined in /etc/nixos/onshape_key/onshape-keys.nix\033[0m"
    else
      export ONSHAPE_ACCESS_KEY="$ACCESS_KEY"
      export ONSHAPE_SECRET_KEY="$SECRET_KEY"
    fi

    echo "✔ OpenSCAD, Onshape-to-Robot and Urdf-Inertia-Script are ready with Python ${python.version} 🐍"
    echo " ! Note: relative paths will not work for these scripts! Start any file path at /"
    echo "OpenSCAD usage: onshape-to-robot-edit-shape <stl-file-path>"
    echo "Oneshape-to-Robot usage: onshape-to-robot <config.json-folder-path>"
    echo "Urdf-Inertia-Script usage: urdf-modifier <urdf-file-path> OR inertia-calc <stl-file-path>"
  '';
}
