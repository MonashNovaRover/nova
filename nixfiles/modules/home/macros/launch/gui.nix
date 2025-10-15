# run gui and rosbridge
{ 
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
in
{
  gui = bashBuilder gui-setup "run-gui";
}
