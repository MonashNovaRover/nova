{ config, lib, modulesPath, ... }:

{
  imports = [
    (modulesPath + "/virtualisation/docker-image.nix")
  ];

  # Undo some changes made by the minimal profile
  environment.noXlibs = false;
  programs.command-not-found.enable = true;

  # Networking
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
