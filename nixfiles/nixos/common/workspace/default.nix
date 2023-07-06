{ config, lib, ... }:

{
  # Note: Ideally, the workspace should be configured entirely in the Home Manager
  # module for cross-distro accesibility.
  # The purpose of this module is to configure defaults based on NixOS settings.

  home-manager.nova.sharedModules = [{
    nova.workspace = {
      gui.enable = lib.mkDefault config.services.xserver.enable;
    };
  }];
}
