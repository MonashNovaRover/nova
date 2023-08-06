{ mkYarnPackage
, fetchFromGitHub
, fetchYarnDeps
}:

mkYarnPackage rec {
  pname = "ros-typescript-generator";
  version = "1.6.4";

  src = fetchFromGitHub {
    owner = "Greenroom-Robotics";
    repo = pname;
    rev = "v${version}";
    hash = "sha256-GeBZ04sWZtL7RyrZ/RPnvi3aODhNcsTwlakt6dpTI68=";
  };

  buildPhase = ''
    runHook preBuild

    export HOME="$(mktemp -d)"
    yarn --offline build

    runHook postBuild
  '';

  doDist = false;
}
