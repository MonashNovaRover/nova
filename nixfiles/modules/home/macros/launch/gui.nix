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
  gui-setup = {
    pre = pre-shell {payload-name="Nova Gui";};
    terminals = [
      {name = "Base:Gui"; platform=base-nix "gui-shell"; cmd="gui-link; gui-run";}
      {name = "Base:Rosbridge"; platform=base; cmd="gui-rosbridge";}
    ];
    post = "${pkgs.xdg-utils}/bin/xdg-open http://localhost:5173/$GUI_ROUTE\n" + post-shell; 
    buildInputs = [ pkgs.xdg-utils ];
    optional-args = [ {letter="r"; variable="GUI_ROUTE"; default="";} ];
  };
  local-gui-setup = { # no aliases, with options to change paths etc
    pre = pre-shell {payload-name="Nova Gui";};
    terminals = [
      {name = "Base:Gui"; platform=base-nix "nix-shell $NOVA_REPO_PATH/nixfiles -A pkgs.ros.nova-gui"; cmd="ln -sf \"$ROS_TS_DEFINITIONS\" $NOVA_REPO_PATH/src/ros/nova-gui/nova-gui/src/ros/rosTypes.ts; cd $NOVA_REPO_PATH/src/ros/nova-gui/nova-gui; yarn dev";}
      {name = "Base:Rosbridge"; platform=base; cmd="gui-rosbridge";}
    ];
    post = "${pkgs.xdg-utils}/bin/xdg-open http://localhost:5173/$GUI_ROUTE\n" + post-shell; 
    buildInputs = [ pkgs.xdg-utils ];
    optional-args = [ {letter="r"; variable="GUI_ROUTE"; default="";} {letter="n"; variable="NOVA_REPO_PATH"; default="/home/nova/nova";} ];
  };
in
{
  gui = bashBuilder gui-setup "run-gui";
  gui-local = bashBuilder local-gui-setup "run-gui-local";
}
