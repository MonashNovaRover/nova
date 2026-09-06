{ stdenv,
  fetchYarnDeps,
  yarnConfigHook,
  yarnBuildHook,
  yarnInstallHook,
  nodejs,
}:

stdenv.mkDerivation {
  name = "reolink-ctl";
  version = "1.0.0";

  src = ./src;

  yarnOfflineCache = fetchYarnDeps {
    yarnLock = ./src/yarn.lock;
    hash = "sha256-noFsgfDrGE712okHCBuifJoT8WOtF/rpKmE51hCL5Hk=";
  };

  nativeBuildInputs = [
    yarnConfigHook
    yarnBuildHook
    yarnInstallHook
    nodejs
  ];

  dontBuild = true;
}
