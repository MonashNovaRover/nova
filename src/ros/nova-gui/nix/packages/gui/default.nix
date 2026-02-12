{ lib
, buildEnv
, mkYarnPackage
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
  serve-gui-script = writers.writePython3 "serve-gui" { doCheck = false; } (builtins.readFile ../../../serve.py);
in
mkYarnPackage {
  name = "gui";

  src = builtins.path rec {
    name = "gui";
    path = ../../../nova-gui;
    filter = lib.novaSourceFilter [ "node_modules" "dist" ] path;
  };

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

    # without this, the built css file is missing 2/3rds of the content
    # maybe the deps we pull in aren't specifically marked as deps of nova-gui
    rm deps/nova-gui/node_modules
    ln -s "$PWD/node_modules" deps/nova-gui/node_modules

    yarn --offline build

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    mkdir -p "$out/share/nova-gui"
    cp -r deps/nova-gui/dist "$out/share/nova-gui/www"

    mkdir -p "$out/bin/"

    echo "#!/bin/bash
    ${serve-gui-script} \"$out/share/nova-gui/www\" \$@" > "$out/bin/serve-gui"
    chmod +x "$out/bin/serve-gui"

    runHook postInstall
  '';

  postInstall = ''
    mkdir -p "$out/nix-support"
    echo doc GUI "$out/share/nova-gui/www/index.html" > "$out/nix-support/hydra-build-products"
  '';

  distPhase = "true";
  passthru.workspacePackages = {
    inherit rosbridge-server;
  };
}
