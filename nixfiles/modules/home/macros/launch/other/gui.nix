# run gui and rosbridge
{ 
    pkgs,
    base,
    pre-shell,
    post-shell,
    bashBuilder,
    base-nix
}:

let 
  delayed-browser-launch = "(echo \"Opening browser in 10 seconds...\";sleep 10; ${pkgs.xdg-utils}/bin/xdg-open http://localhost:5173/$GUI_ROUTE) & \n";
  nova-repo-flag = {letter="n"; variable="NOVA_REPO_PATH"; default="/home/nova/nova"; description="Usage: -n <path/to/nova/repo>";};
  gui-route-flag = {letter="r"; variable="GUI_ROUTE"; default=""; description="Additional path appended to the GUI URL on open - Usage: -r <path/thing.html>";};
  tile-file-flag = {letter="t"; variable="TILE_FILE"; default=""; description="Usage: -t <path/to/.mbtiles>"; required = true;};

  gui-setup = {
    pre = pre-shell {payload-name="Nova Gui";};
    terminals = [
      {name = "Base:Gui"; platform=base-nix "gui-shell"; cmd="gui-link; gui-run";}
      {name = "Base:Rosbridge"; platform=base; cmd="gui-rosbridge";}
    ];
    # open web browser after 10 seconds while continuing with post-shell
    post = delayed-browser-launch + post-shell; 
    buildInputs = [ pkgs.xdg-utils ];
    flag-args = [ gui-route-flag ];
  };

  gui-local-setup = gui-setup // { # no aliases, with options to change paths etc
    terminals = [
      {name = "Base:Gui"; platform=base-nix "nix-shell $NOVA_REPO_PATH/nixfiles -A pkgs.ros.nova-gui"; cmd="ln -sf \"$ROS_TS_DEFINITIONS\" $NOVA_REPO_PATH/src/ros/nova-gui/nova-gui/src/ros/rosTypes.ts; cd $NOVA_REPO_PATH/src/ros/nova-gui/nova-gui; yarn dev";}
      {name = "Base:Rosbridge"; platform=base; cmd="./ros2 launch rosbridge_server rosbridge_websocket_launch.xml";}
    ];
    flag-args = gui-setup.flag-args ++ [ nova-repo-flag ];
  };
  
  gui-maps-setup = gui-local-setup // {
    pre = pre-shell {payload-name="Nova GUI + Tileserver";};
    terminals = gui-local-setup.terminals ++ [
      {
        name = "Base:Tileserver"; 
        platform=base-nix "nix-shell $NOVA_REPO_PATH/nixfiles -A pkgs.ros.nova-gui"; 
        cmd="ln -s $NOVA_REPO_PATH/src/ros/nova-gui/nova-gui/node_modules/tileserver-gl-styles $NOVA_REPO_PATH/src/ros/nova-gui/nova-gui/node_modules/tileserver-gl-light/node_modules/tileserver-gl-styles; yarn --cwd $NOVA_REPO_PATH/src/ros/nova-gui/nova-gui tileserver-gl-light --file $TILE_FILE";
      }
    ];
    flag-args = gui-local-setup.flag-args ++ [ tile-file-flag ];
  };
in
{
  gui = bashBuilder gui-setup "run-gui";
  gui-local = bashBuilder gui-local-setup "run-gui-local";
  gui-maps = bashBuilder gui-maps-setup "run-gui-maps";
}
