{ config, lib, ... }:

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
    hardware.nvidia-jetpack = lib.mkSinkUndeclaredOptions { };
  };

  config = lib.mkIf cfg.enable ({
    nixpkgs.stdenv.hostPlatform = "aarch64-linux";
    hardware.nvidia-jetpack.enable = true;

    assertions = [{
      assertion = hasJetpackChannel;
      message = "The jetpack-nixos channel is not available! It must be added.";
    }];
  });
}
