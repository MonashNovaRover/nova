{ modulesPath, ... }:

{
  imports = [
    ./nova-installation-cd-base.nix
    ((if modulesPath == "" then <nixpkgs> + /nixos/modules else modulesPath) + /installer/cd-dvd/installation-cd-graphical-base.nix)
  ];

  nova.desktop.enable = true;
}
