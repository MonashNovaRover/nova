{ lib
, buildEnv
, mkYarnPackage
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
, static-web-server
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
    ${static-web-server}/bin/static-web-server --root $out/share/nova-gui/www --page-fallback $out/share/nova-gui/www/index.html --cache-control-headers false \$@
    err=\$?
    if [ \$err -ne 0 ]; then
      echo If it says permission denied, maybe you want to specify a non-default port, e.g.
      echo -e \\\\t \$0 -p 8080
    fi
    exit \$err
    " > "$out/bin/serve-gui"
    chmod +x "$out/bin/serve-gui"

    runHook postInstall
  '';

  distPhase = "true";
  passthru.workspacePackages = {
    inherit rosbridge-server;
  };
}
