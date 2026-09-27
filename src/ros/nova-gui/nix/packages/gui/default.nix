{ 
  buildEnv, 
  fetchYarnDeps, 
  lib, 
  nodejs, 
  nova-arm-interfaces, 
  nova-blcmd-interfaces, 
  nova-camera-msgs, 
  nova-cmd-interfaces, 
  nova-drive-interfaces, 
  nova-input-interfaces, 
  nova-interfaces, 
  nova-science-interfaces, 
  ros-core, 
  ros-typescript-definitions, 
  rosbridge-server, 
  stdenv, 
  writers, 
  yarnBuildHook, 
  yarnConfigHook, 
  yarnInstallHook, 
}:

let
  # ROS packages for message generation
  rosMessagePackages = [
    nova-arm-interfaces
    nova-blcmd-interfaces
    nova-camera-msgs
    nova-cmd-interfaces
    nova-drive-interfaces
    nova-input-interfaces
    nova-interfaces
    nova-science-interfaces
    ros-core
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
    hash = "sha256-fHq9S4lLBhV/IzHr+FpsF7mdBmp26eqg8PwvqB5ZsDY=";
  };

  nativeBuildInputs = [
    nodejs
    yarnBuildHook
    yarnConfigHook
    yarnInstallHook
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

  preBuild = ''
    # without this, the built css file is missing 2/3rds of the content
    # maybe the deps we pull in aren't specifically marked as deps of nova-gui
    mkdir -p deps/nova-gui
    ln -s "$PWD/node_modules" deps/nova-gui/node_modules
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
