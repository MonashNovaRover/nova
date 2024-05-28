{ mkYarnPackage
, fetchFromGitHub
, fetchYarnDeps
}:

mkYarnPackage rec {
   pname = "tileserver-gl";
   version = "4.11.1";

  src = fetchFromGitHub {
     owner = "maptiler";
     repo = "tileserver-gl";
     rev = "v4.11.1"; 
     sha256 = "IZZq2trDn3HBUI4SU1rplIb1nv5mB7O4bxaAmX5M/W0=";
   };

  buildPhase = ''
    runHook preBuild

    export HOME="$(mktemp -d)"
    yarn --offline build

    runHook postBuild
  '';

  doDist = false;
}
