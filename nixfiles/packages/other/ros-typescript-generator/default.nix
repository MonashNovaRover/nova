{ lib
, stdenv
, fetchFromGitHub
, fetchYarnDeps
, yarnConfigHook
, yarnBuildHook
, nodejs
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "ros-typescript-generator";
  version = "1.7.0";

  src = fetchFromGitHub {
    owner = "Greenroom-Robotics";
    repo = finalAttrs.pname;
    tag = "v${finalAttrs.version}";
    hash = "sha256-cw14FDLHIxfjKnYN1WWNCXHIPsgdSBh+NyxFzQVhPlw=";
  };

  yarnOfflineCache = fetchYarnDeps {
    yarnLock = finalAttrs.src + "/yarn.lock";
    hash = "sha256-ZDL6obywwi/hWwbG5d6gvx93Wo6mPtCwz11+I5/Bllw=";
  };

  nativeBuildInputs = [
    yarnConfigHook
    yarnBuildHook
    nodejs
  ];

  postPatch = ''
    export HOME="$(mktemp -d)"
  '';

  meta = {
    description = "Generate TypeScript types from ROS message definitions";
    license = lib.licenses.asl20;
  };
})
