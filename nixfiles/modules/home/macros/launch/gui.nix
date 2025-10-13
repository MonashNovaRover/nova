# run gui and rosbridge
{ 
    base,
    pre-shell,
    post-shell,
    make-shell,
    base-nix,
    route
}:

let 
  gui-setup = {
    pre-shell = pre-shell {payload-name="Nova Gui";};
    terminals = [
      {name = "Base:Gui"; platform=base-nix "gui-shell"; cmd="gui-link; gui-run";}
      {name = "Base:Rosbridge"; platform=base; cmd="gui-rosbridge";}
    ];
    post-shell = "xdg-open http://localhost:5173/${route}\n" + post-shell;
  };
in
{
  default = make-shell gui-setup;
}
