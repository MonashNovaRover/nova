{ mkShell
, config
, nova-gui
, pkgs
, yarn
}:

# This shell is designed for use in development and it is assumed the developer will run the gui using yarn
# If looking for a shell with the gui fully built, use the default.nix instead (e.g for gui-serve)

let 

gui = nova-gui.overrideAttrs(oldAttrs :{
  # disable building of gui-serve to make it faster
  postUnpack = "";
  buildPhase = ''
    runHook preBuild
    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall
    runHook postInstall
  '';

  # add path to node_modules to output to be used in dev shell
  configurePhase = oldAttrs.configurePhase + ''
    mkdir -p $out
    echo $node_modules > $out/node_modules
    echo $ROS_TS_DEFINITIONS > $out/ROS_TS_DEFINITIONS
  '';

  passthru.ROS_TS_DEFINITIONS = oldAttrs.ROS_TS_DEFINITIONS;
});

diff_ignore=''-x ".vite" -x ".vite-temp" -x "nova-gui"'';

in

mkShell {
  preBuild = ''
    echo "Entering GUI shell..."
  '';
  buildInputs = [ yarn gui ];

  shellHook = ''
    if ! diff -rq $(cat ${gui}/node_modules) \${builtins.toString ../../../nova-gui/node_modules} ${diff_ignore} >/dev/null; then
      echo "Copying nix-store gui modules $(cat ${gui}/node_modules) to ${builtins.toString ../../../nova-gui/node_modules}"
      cp -rf $(cat ${gui}/node_modules) ${builtins.toString ../../../nova-gui}
      chmod -R 777 ${builtins.toString ../../../nova-gui/node_modules}
    else
      echo "Node modules has not changed, skipping copy..."
    fi

    echo "Linking Typescript Definitions... "
    ln -sf $(cat ${gui}/ROS_TS_DEFINITIONS) ${builtins.toString ../../../nova-gui/src/ros/rosTypes.ts}
    
    cd ${toString ../../../nova-gui}

    echo -e "Done! Run \e[40m\e[1;93mgui-run\e[0m to run the GUI and don't forget to run \e[40m\e[1;93mgui-rosbridge\e[0m"
  '';
}
