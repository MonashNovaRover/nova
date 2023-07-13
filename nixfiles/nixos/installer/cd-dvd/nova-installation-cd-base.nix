{ config, pkgs, lib, modulesPath, ... }:

{
  imports = [
    ((if modulesPath == "" then <nixpkgs> + /nixos/modules else modulesPath) + /installer/cd-dvd/installation-cd-base.nix)
  ];

  system.nixos.tags = [ "nova" ];
  isoImage = rec {
    edition = lib.mkDefault "nova";
    appendToMenuLabel = " Live System";
    efiSplashImage = splashImage;
    splashImage = "${pkgs.nova.nova-backgrounds}/share/backgrounds/nova/logo-dark.png";
    graphicalGrub = lib.mkDefault true;
  };

  # For some reason, including documentation crashes the build of the
  # "lazy-options.json" derivation due to "Argument list too long" when starting
  # the builder.
  documentation.nixos.enable = lib.mkForce false;

  nova = {
    profile = "shared";
    desktop.enable = lib.mkOverride 900 false;
  };

  home-manager.sharedModules = [{
    nova = {
      workspace.enable = lib.mkDefault false;
    };
  }];

  home-manager.users.nova.home.stateVersion = config.system.nixos.release;
}
