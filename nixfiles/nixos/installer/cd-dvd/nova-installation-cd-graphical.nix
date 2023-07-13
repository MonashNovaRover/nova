{ modulesPath, ... }:

{
  imports = [
    ./nova-installation-cd-base.nix
    (modulesPath + /installer/cd-dvd/installation-cd-graphical-base.nix)
  ];

  services.xserver.displayManager.autoLogin = {
    enable = true;
    user = "nova";
  };

  nova.desktop.enable = true;
}
