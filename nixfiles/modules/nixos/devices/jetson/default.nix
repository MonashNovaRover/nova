{ config, lib, pkgs, ... }:

let
  cfg = config.devices.jetson;
  jetpack-nixos-module = (import (builtins.toPath "${<jetpack-nixos>}/modules/default.nix") (import ( builtins.toPath "${jetpack-nixos}/overlay.nix")));
  hasJetpackChannel = (builtins.tryEval <jetpack-nixos>).success;
in
{
  imports = [
    (lib.optionalAttrs hasJetpackChannel jetpack-nixos-module)
    ./boot
    ./devkit
    ./peripherals
    ./devices
  ];

  options = {
    devices.jetson.enable = lib.mkEnableOption "configuration for NVIDIA Jetson SoMs" // { internal = true; };
  } // lib.optionalAttrs (!hasJetpackChannel) {
    hardware.nvidia-jetpack = lib.mkOption {
      description = "Modules for Jetpack 6 as a flake";
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
