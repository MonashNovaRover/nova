{ config, lib, modulesPath, ... }:

{
  imports = [
    (modulesPath + "/virtualisation/docker-image.nix")
  ];

  system.nixos.tags = [ "docker" ];

  # Undo some changes made by the minimal profile
  environment.noXlibs = false;
  programs.command-not-found.enable = true;

  # Networking
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
