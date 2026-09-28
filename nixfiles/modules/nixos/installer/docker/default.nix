{ config, lib, modulesPath, ... }:

{
  imports = [
    (modulesPath + "/virtualisation/docker-image.nix")
  ];

  system.nixos.tags = [ "docker" ];

  # Networking
  networking.hostName = "";
  networking.useHostResolvConf = false;
  networking.dhcpcd.enable = false;
  services.resolved.enable = true;

  nova = {
    profile = "shared";
    desktop.enable = false;
  };

  home-manager.nova.sharedModules = [{
    home.stateVersion = lib.mkDefault config.system.nixos.release;
    nova = {
      workspace.enable = false;
    };
  }];
}
