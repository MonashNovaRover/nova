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
  passthru.workspacePackages = {
    inherit rosbridge-server;
  };
}
