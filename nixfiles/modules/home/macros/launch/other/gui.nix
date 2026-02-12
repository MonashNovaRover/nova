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
  delayed-browser-launch = "(echo \"Opening browser in 2 seconds...\";sleep 2; ${pkgs.xdg-utils}/bin/xdg-open http://localhost:5173/$GUI_ROUTE) & \n";
  gui-route-flag = {letter="r"; variable="GUI_ROUTE"; default=""; description="Additional path appended to the GUI URL on open - Usage: -r <path/thing.html>";};

  gui-setup = {
    pre = pre-shell {payload-name="Nova Gui";};
    terminals = [
      {name = "Base:Gui"; platform=base; cmd="./serve-gui 5173";}
      {name = "Base:Rosbridge"; platform=base; cmd="./ros2 launch rosbridge_server rosbridge_websocket_launch.xml";}
    ];
    # open web browser after 2 seconds while continuing with post-shell
    post = delayed-browser-launch + post-shell; 
    buildInputs = [ pkgs.xdg-utils ];
    flag-args = [ gui-route-flag ];
  };

  gui-maps-setup = gui-setup // {
    pre = pre-shell {payload-name="Nova GUI + Tileserver";};
    terminals = gui-setup.terminals ++ [
      {
        name = "Base:Tileserver";
        platform=base;
        cmd="./mbtileserver -p 8080 --missing-image-tile-404 -d ~/tiles";
      }
    ];
    flag-args = gui-setup.flag-args;
  };
in
{
  gui = bashBuilder gui-setup "run-gui";
  gui-maps = bashBuilder gui-maps-setup "run-gui-maps";
}
