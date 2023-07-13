{ modulesPath, ... }:

{
  imports = [
    ./nova-installation-cd-base.nix
    (modulesPath + /installer/cd-dvd/installation-cd-graphical-base.nix)
  ];

  nova.desktop.enable = true;
}
