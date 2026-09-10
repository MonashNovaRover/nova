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

  postInstall = ''
    mkdir -p "$out/share/nova-gui"
    cp -r dist "$out/share/nova-gui/www"

    mkdir -p "$out/bin/"

    echo "#!/bin/bash
    ${serve-gui-script} \"$out/share/nova-gui/www\" \$@" > "$out/bin/gui-serve"
    chmod +x "$out/bin/gui-serve"
  '';

  distPhase = "true";
  passthru.workspacePackages = {
    inherit rosbridge-server;
  };

  shellHook = ''
    export oldDir=$(pwd)
    cd ${toString ../../../nova-gui}

    # auto install node_modules from the offline cache
    # inspired by https://github.com/NixOS/nixpkgs/blob/c27cdad491a991b11ed731760aa2ef8db0cb0410/pkgs/build-support/node/fetch-yarn-deps/yarn-config-hook.sh
    echo -e "\e[33mInstalling node_modules from yarn offline cache...\e[0m"
    cp yarn.lock yarn.lock.keep
    yarn config --offline set yarn-offline-mirror "$yarnOfflineCache"
    fixup-yarn-lock yarn.lock
    yarn install \
        --frozen-lockfile \
        --force \
        --production=false \
        --ignore-engines \
        --ignore-platform \
        --ignore-scripts \
        --non-interactive \
        --offline \
    rm yarn.lock
    mv yarn.lock.keep yarn.lock
    yarn config delete yarn-offline-mirror

    # automatically link generated ROS TypeScript definitions to rostypes.ts
    echo -e "\e[33mLinking generated ROS TypeScript definitions\e[0m"
    rm src/ros/rosTypes.ts
    ln -s "$ROS_TS_DEFINITIONS" "src/ros/rosTypes.ts"

    cd $oldDir

    echo -e "\e[32mRun the gui in dev mode with \e[0m\e[37;41mgui-run\e[0m\n\e[32mTo build the production version gui, run: \e[0m\e[37;41mgui-build\e[0m\n\e[33mDon't forget to run Rosbridge! \e[0m\e[37;41mgui-rosbridge;\e[0m"

    # for some reason this shell likes to print out the yarn output again when it exits. Not harmful but I can't seem to fix it and i've spent half an hour on it already so i give up
  '';
}
