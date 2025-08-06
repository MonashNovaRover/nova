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
    version = "1.7.6";

    src = pkgs.fetchFromGitHub {
      owner = "Rhoban";
      repo = pname;
      rev = "65250f0e7044f56a2e2d59a09b14e5b2c591cf0b";
      hash = "sha256-lTkAnM8uvrwWJsBx865ReZ/DmdyrQIjLsylAuCwBk+c=";
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
    pyPkgs.pymeshlab
  ];

  shellHook = ''
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
  '';
}
