# run gui and rosbridge
{ 
    pkgs,
    base,
    pre-shell,
    post-shell,
    bashBuilder,
    base-nix,
    route
}:

let 
  gui-setup = {
    pre = pre-shell {payload-name="Nova Gui";};
    terminals = [
      {name = "Base:Gui"; platform=base-nix "gui-shell"; cmd="gui-link; gui-run";}
      {name = "Base:Rosbridge"; platform=base; cmd="gui-rosbridge";}
    ];
    post = "xdg-open http://localhost:5173/${route}\n" + post-shell;
  };
  local-gui-setup = { # no aliases
    pre = pre-shell {payload-name="Nova Gui";};
    terminals = [
      {name = "Base:Gui"; platform=base-nix "gui-shell"; cmd="gui-link; gui-run";}
      {name = "Base:Rosbridge"; platform=base; cmd="gui-rosbridge";}
    ];
    post = "${pkgs.xdg-utils}/bin/xdg-open http://localhost:5173/${route}\n" + post-shell; 
    buildInputs = [ pkgs.xdg-utils ];
  };
in
{
  gui = bashBuilder gui-setup "run-gui";
  gui-local = bashBuilder local-gui-setup "run-gui-local";
}
