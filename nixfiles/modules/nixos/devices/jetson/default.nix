{ config, lib, pkgs, ... }:

let
  cfg = config.devices.jetson;
  jetpack-nixos = if (builtins.tryEval <jetpack-nixos>).success then <jetpack-nixos> else builtins.fetchTarball {
    # At this point, pkgs.fetchFromGitHub is not real.
    # Should use nixfiles/revisions.json
    url = "https://github.com/anduril/jetpack-nixos/archive/4d857d6da48c420da39964b6fb7ba23a958abad3.tar.gz";
    sha256 = "0a8vymdkzdnsmpp3iy95x9y7zczmzr0mzpazz9xm67ki84y2sb1m"; # TODO
  };
  jetpack-nixos-module = (import (builtins.toPath "${jetpack-nixos}/modules/default.nix") (import ( builtins.toPath "${jetpack-nixos}/overlay.nix")));
  hasJetpackChannel = (builtins.tryEval jetpack-nixos).success;
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
