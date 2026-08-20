{ lib
, stdenv
, buildEnv
, fetchYarnDeps
, yarnConfigHook
, yarnBuildHook
, writers
, rosbridge-server
, ros-typescript-definitions
, ros-core
, nova-drive-interfaces
, nova-blcmd-interfaces
, nova-arm-interfaces
, nova-input-interfaces
, nova-cmd-interfaces
, nova-interfaces
, nova-camera-msgs
, nova-science-interfaces
, nodejs
}:

let
  # ROS packages for message generation
  rosMessagePackages = [
    ros-core
    nova-drive-interfaces
    nova-blcmd-interfaces
    nova-arm-interfaces
    nova-input-interfaces
    nova-cmd-interfaces
    nova-interfaces
    nova-camera-msgs
    nova-science-interfaces
  ];
  serve-gui-script = writers.writePython3 "gui-serve" { doCheck = false; } (builtins.readFile ../../../serve.py);
in
stdenv.mkDerivation {
  name = "gui";

  # Make sure that the node modules derivation doesn't have the whole source
  # folder as an input (i.e. changes to tsx files won't trigger rebuilding node_modules)
  src = builtins.path rec {
    name = "gui";
    path = ../../../nova-gui;
    filter = lib.novaSourceFilter [ "node_modules" "dist" ] path;
  };

  yarnOfflineCache = fetchYarnDeps {
    yarnLock = ../../../nova-gui/yarn.lock;
    hash = "sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
  };

  nativeBuildInputs = [
    yarnConfigHook
    yarnBuildHook
    nodejs
  ];

  ROS_TS_DEFINITIONS = (ros-typescript-definitions.override {
    typePrefix = "IRos";
    rosEnv = (buildEnv {
      wrapPrograms = false;
      paths = rosMessagePackages;
    }).overrideAttrs {
      name = "gui-ros-env";
    };
  }) + "/share/ros-typescript-definitions/messages.ts";

  postUnpack = ''
    ln -s "$ROS_TS_DEFINITIONS" "$sourceRoot/src/ros/rosTypes.ts"
  '';

  buildPhase = ''
    runHook preBuild

    export HOME="$(mktemp -d)"
    yarn --offline build

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    mkdir -p "$out/share/nova-gui"
    cp -r dist "$out/share/nova-gui/www"

    mkdir -p "$out/bin/"

    echo "#!/bin/bash
    ${serve-gui-script} \"$out/share/nova-gui/www\" \$@" > "$out/bin/gui-serve"
    chmod +x "$out/bin/gui-serve"

    runHook postInstall
  '';

  distPhase = "true";
  passthru.workspacePackages = {
    inherit rosbridge-server;
  };
}
