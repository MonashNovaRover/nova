{ pkgs ? import <nixpkgs> { } }:

let
  # Fetch a specific version of nixpkgs for MeshLab 2020.07
  olderNixpkgs = import
    (builtins.fetchTarball {
      url = "https://github.com/NixOS/nixpkgs/archive/5c1ffb7a9fc96f2d64ed3523c2bdd379bdb7b471.tar.gz";
      sha256 = "081ca1ygnm76gn2igyry0fgzv5ar4c50mcg8j8vrpbrp36li9lw6";
    })
    { };

  # Use the older version of MeshLab
  meshlab = olderNixpkgs.meshlab;

  # Define onshape-to-robot build
  onshapeToRobot = pkgs.python3Packages.buildPythonApplication rec {
    pname = "onshape-to-robot";
    version = "0.3.26";

    src = pkgs.fetchPypi {
      inherit pname version;
      hash = "sha256-aMxfZw1B/J1NgRbbUXRwjM2hEbUlTXfVIt9r2dVtOuY=";
    };

    buildInputs = with pkgs.python3Packages; [
      numpy
      pybullet
      requests
      commentjson
      colorama
      numpy-stl
      transforms3d
    ];

    nativeBuildInputs = [
      pkgs.pkg-config
    ];

    postPatch = ''
      substituteInPlace onshape_to_robot/edit_shape.py \
        --replace "os.system('cd '+directory+'; openscad '+os.path.basename(fileName)+')'" \
        'subprocess.run(["openscad", os.path.basename(fileName)], cwd=directory, check=True)'

        substituteInPlace onshape_to_robot/config.py \
        --replace "/usr/bin/meshlabserver" "${meshlab}/bin/meshlabserver"
    '';

    meta = with pkgs.lib; {
      description = "A Python library to convert Onshape assemblies into URDF or SDF robots";
      homepage = "https://github.com/Rhoban/onshape-to-robot";
      license = licenses.mit;
    };
  };

in
pkgs.mkShell {
  buildInputs = [
    pkgs.openscad
    meshlab
    onshapeToRobot
    pkgs.python3Packages.numpy
    pkgs.python3Packages.pybullet
    pkgs.python3Packages.requests
    pkgs.python3Packages.commentjson
    pkgs.python3Packages.colorama
    pkgs.python3Packages.numpy-stl
    pkgs.python3Packages.transforms3d
  ];

  # Set up PYTHONPATH to include all dependencies
  shellHook = ''
    export PATH=${onshapeToRobot}/bin:$PATH
    export PYTHONPATH=${pkgs.python3Packages.python.sitePackages}:${pkgs.python3Packages."numpy-stl"}/lib/python3.12/site-packages:$PYTHONPATH
    echo "OpenSCAD, MeshLab (2020.07), and Onshape-to-Robot are ready to use."
  '';
}
