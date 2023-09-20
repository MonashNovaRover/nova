{ config, lib, ... }:

let
  cfg = config.devices.jetson;
  hasJetpackChannel = (builtins.tryEval <jetpack-nixos>).success;
in
{
  imports = [
    (lib.optionalAttrs hasJetpackChannel <jetpack-nixos/modules>)
    ./devkit
    ./orin-nano
  ];

  options.devices.jetson.enable = lib.mkEnableOption "configuration for NVIDIA Jetson SoMs" // { internal = true; };

  config = lib.mkIf (cfg.enable) (if hasJetpackChannel then {
    nixpkgs.hostPlatform = "aarch64-linux";
    hardware.nvidia-jetpack.enable = true;
  } else {
    assertions = [
      {
        assertion = false;
        message = "The jetpack-nixos channel is not available! It must be added.";
      }
    ];
  });
}
