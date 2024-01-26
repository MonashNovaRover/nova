{ lib
, symlinkJoin
, buildEnv
, mkYarnPackage
, rosbridge-server
, ros-typescript-definitions
, ros-core
, nova-core
}:

let
  # ROS packages for message generation
  rosMessagePackages = [
    ros-core
    nova-core
  ];
in
mkYarnPackage {
  name = "gui";

  src = builtins.path rec {
    name = "gui";
    path = ../../../nova-gui;
    filter = lib.novaSourceFilter [ ] path;
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
    ln -s "$ROS_TS_DEFINITIONS" "$sourceRoot/src/ros/rosMessageTypes.ts"
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
