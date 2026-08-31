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

  installPhase = ''
    runHook preInstall
    mkdir -p $out/lib/node_modules/ros-typescript-generator
    cp -r build $out/lib/node_modules/ros-typescript-generator/
    cp -r .bin $out/lib/node_modules/ros-typescript-generator/
    cp package.json $out/lib/node_modules/ros-typescript-generator/
    mkdir -p $out/bin
    ln -s $out/lib/node_modules/ros-typescript-generator/.bin/ros-typescript-generator $out/bin/ros-typescript-generator
    patchShebangs $out
    runHook postInstall
  '';

  meta = {
    description = "Generate TypeScript types from ROS message definitions";
    license = lib.licenses.asl20;
  };
})
