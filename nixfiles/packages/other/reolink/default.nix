{ lib
, stdenvNoCC
, fetchYarnDeps
, yarnConfigHook
, yarnInstallHook
, nodejs
}:

stdenvNoCC.mkDerivation {
  pname = "reolink-ctl";
  version = "0.0.1";

  src = ./src;

  yarnOfflineCache = fetchYarnDeps {
    yarnLock = ./src/yarn.lock;
    hash = "sha256-noFsgfDrGE712okHCBuifJoT8WOtF/rpKmE51hCL5Hk=";
  };

  nativeBuildInputs = [
    yarnConfigHook
    yarnInstallHook
    nodejs
  ];

  dontYarnBuild = true;

  meta = with lib; {
    description = "Reolink ONVIF control utility";
    license = licenses.asl20;
  };
}
