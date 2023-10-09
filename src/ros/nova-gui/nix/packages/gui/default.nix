{ lib
, mkYarnPackage
, fetchYarnDeps
, nodejs_20
, yarn-berry
}:

mkYarnPackage {
  name = "gui";
  
  # offlineCache = fetchYarnDeps {
  #   yarnLock = src + "/yarn.lock";
  #   hash = "....";
  # };

  src = builtins.path rec {
    name = "gui";
    path = ../../../nova-gui;
    filter = lib.novaSourceFilter [ ] path;
  };

  buildPhase = ''
    runHook preBuild

    yarn --offline build

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    mkdir -p "$out/share/nova-gui"
    cp -r deps/nova-gui/dist "$out/share/nova-gui/www"

    runHook postInstall
  '';

  distPhase = "true";
}