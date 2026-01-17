{ config, lib, pkgs, ... }:

let
  cfg = config.devices.jetson;
  hasJetpackChannel = (builtins.tryEval <jetpack-nixos>).success;
in
{
  imports = [
    (lib.optionalAttrs hasJetpackChannel <jetpack-nixos/modules>)
    ./boot
    ./devkit
    ./peripherals
    ./devices
  ];

  options = {
    devices.jetson.enable = lib.mkEnableOption "configuration for NVIDIA Jetson SoMs" // { internal = true; };
  } // lib.optionalAttrs (!hasJetpackChannel) {
    #hardware.nvidia-jetpack = lib.mkSinkUndeclaredOptions { };
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
