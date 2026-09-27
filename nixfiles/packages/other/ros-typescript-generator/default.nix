{
  fetchFromGitHub, 
  fetchYarnDeps, 
  nodejs, 
  stdenv, 
  yarnBuildHook, 
  yarnConfigHook, 
  yarnInstallHook, 
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "ros-typescript-generator";
  version = "1.10.0";

  src = fetchFromGitHub {
    owner = "Greenroom-Robotics";
    repo = finalAttrs.pname;
    tag = "v${finalAttrs.version}";
    hash = "sha256-R9orKPGpzfJG8XDXfOcFQeaTh0gRaWtGaAZMbb72vu8=";
  };

  yarnOfflineCache = fetchYarnDeps {
    yarnLock = "${finalAttrs.src}/yarn.lock";
    hash = "sha256-e4IrZUQ78abLgAhT7SlqXlFa6io7B/U73afYFaLhWg8=";
  };

  nativeBuildInputs = [
    nodejs 
    yarnBuildHook 
    yarnConfigHook 
    yarnInstallHook 
  ];
})
