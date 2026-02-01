{ config, lib, pkgs, ... }:

let
  cfg = config.devices.jetson;
  hasJetpackChannel = true;
  jetpack-nixos = builtins.fetchTarball {
    url = "https://github.com/anduril/jetpack-nixos/archive/79a0ba1d5df6bfef19b425169fcb8478ecf2686f.tar.gz";
    sha256 = "1wywzmx452f594gsvbj4207m3fm993mkg0jscidjdj36alalf82a";
  };
  jetpack-nixos-module = (import (builtins.toPath "${jetpack-nixos}/modules/default.nix") (import ( builtins.toPath "${jetpack-nixos}/overlay.nix")));
in
{
  imports = [
    jetpack-nixos-module
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

    assertions = [{
      assertion = hasJetpackChannel;
      message = "The jetpack-nixos channel is not available! It must be added.";
    }];
  });
}
