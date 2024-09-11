{ mkYarnPackage
, fetchFromGitHub
, fetchYarnDeps
}:

mkYarnPackage rec {
  pname = "ros-typescript-generator";
  version = "1.7.0";

  src = fetchFromGitHub {
    owner = "Greenroom-Robotics";
    repo = pname;
    rev = "v${version}";
    hash = "sha256-cw14FDLHIxfjKnYN1WWNCXHIPsgdSBh+NyxFzQVhPlw=";
  };

  buildPhase = ''
    runHook preBuild

    export HOME="$(mktemp -d)"
    yarn --offline build

    runHook postBuild
  '';

  doDist = false;
}
