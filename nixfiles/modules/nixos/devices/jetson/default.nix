{ config, lib, pkgs, ... }:

let
  cfg = config.devices.jetson;
  hasJetpackChannel = (builtins.tryEval <jetpack-nixos>).success
  revisions = builtins.fromJSON (builtins.readFile ../../revisions.json);
  # When hydra builds the devices jobset it passes jetpack as <jetpack-nixos> so that needs to work
  # in the restricted evaluation mode where it can't get stuff from github.
  # When the orin builds it it doesn't have that currently so it needs it to work without it as well.
  # The docs job evalutates the options here too but I forgot how it handles this.
  jetpack-nixos = if hasJetpackChannel then <jetpack-nixos> else builtins.fetchTarball {
    url = "https://github.com/anduril/jetpack-nixos/archive/${revisions.jetpack-nixos.rev}.tar.gz";
    sha256 = revisions.jetpack-nixos.hash;
  };
  jetpack-nixos-module = (import (builtins.toPath "${jetpack-nixos}/modules/default.nix") (import ( builtins.toPath "${jetpack-nixos}/overlay.nix")));
in
{
  imports = [
    ./boot
    ./devkit
    ./peripherals
    ./devices
  ] ++ lib.optional (jetpack-nixos != null) jetpack-nixos-module;

  options = {
    devices.jetson.enable = lib.mkEnableOption "configuration for NVIDIA Jetson SoMs" // { internal = true; };
  } // lib.optionalAttrs (!hasJetpackChannel) {
    hardware.nvidia-jetpack = lib.mkOption {
      description = "Modules for Jetpack 6";
      type = with lib.types; attrsOf (submodule {
        freeformType = lib.types.anything;
      });
    };
  };

  config = lib.mkIf cfg.enable ({
    nixpkgs.hostPlatform = "aarch64-linux";
    hardware.nvidia-jetpack.enable = true;
    nova.substituters.nvidia.enable = true;


    # Prevent this spam in journalctl:
    # /etc/udev/rules.d/99-tegra-devices.rules:38 Unknown group 'debug', ignoring
    users.groups = {
      debug = {};
    };

    assertions = [{
      assertion = hasJetpackChannel;
      message = "The jetpack-nixos channel is not available! It must be added.";
    }];
  });
}
