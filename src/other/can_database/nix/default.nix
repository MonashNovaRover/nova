{
  lib,
  stdenv,
  xacro,
  python3Packages,
}:

stdenv.mkDerivation {
  name = "nova-can-database";

  src = builtins.path rec {
    name = "nova-can-database-source";
    path = ../.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [
    python3Packages.cantools
    xacro
  ];

  buildPhase = ''
    runHook preBuild

    buildCANDatabase () {
      fileName=$(basename $0)
      roverName=$( echo $0 | awk -F '/' '/\.\/.*\/networks\/.*/ { print $2 }' )
      roverDir=result/''${roverName}

      printf 'Building CAN database files from source "%s" for rover "%s"...\n' $fileName $roverName
      mkdir -p ''${roverDir}

      fileStem=''${fileName%%.*}

      KCDFilePath=''${roverDir}/''${fileStem}.kcd
      xacro $0 -o ''${KCDFilePath}

      DBCFilePath=''${roverDir}/''${fileStem}.dbc
      cantools convert ''${KCDFilePath} ''${DBCFilePath}
    }
    export -f buildCANDatabase

    find . -path './*/networks/*' -a -name '*.kcd.xacro' \
      -exec bash -c buildCANDatabase {} \;

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    mkdir -p ''${out}/can-database
    cp -r -t ''${out}/can-database result/*

    runHook postInstall
  '';

}
