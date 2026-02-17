{ config, lib, ... }:

let
  cfg = config.devices.raspberry-pi;
  nixos-raspberrypi-src = builtins.fetchTarball {
    # merge this early: https://github.com/nvmd/nixos-raspberrypi/pull/131
    url = "https://github.com/nvmd/nixos-raspberrypi/archive/6f521ab94ff8d8841aea47bf21d82db3eb5e6a43.tar.gz";
    sha256 = "0zf5j5118w4zgxzmrgvmy7r96pb42y3nliwv416vw07qilnhb06r";
  };
  #nixos-raspberrypi-module = (import (builtins.toPath "${nixos-raspberrypi}/modules/raspberry-pi-4.nix"));
  raspberrypi-nixos = (import nixos-raspberrypi-src).outputs;
in
{
  imports = [
    ./devices
    # TODO: they make their modules unconditional :/
    raspberrypi-nixos.nixosModules.raspberry-pi-4.base
    raspberrypi-nixos.nixosModules.nixpkgs-rpi
  ];

  options.devices.raspberry-pi.enable = lib.mkEnableOption "configuration for the raspberry-pi";

  config = lib.mkIf cfg.enable {
    nixpkgs.hostPlatform = "aarch64-linux";

    #nixpkgs.overlays = [
    #  raspberrypi-nixos.overlays.pkgs
    #  raspberrypi-nixos.overlays.bootloader
    #  raspberrypi-nixos.overlays.vendor-kernel
    #  raspberrypi-nixos.overlays.vendor-firmware
    #  raspberrypi-nixos.overlays.kernel-and-firmware
    #  raspberrypi-nixos.overlays.vendor-pkgs
    #];

    nix.settings = {
      substituters = [
        "https://nixos-raspberrypi.cachix.org"
      ];
      trusted-public-keys = [
        "nixos-raspberrypi.cachix.org-1:4iMO9LXa8BqhU+Rpg6LQKiGa2lsNh/j2oiYLNOQ5sPI="
      ];
    };
  };
}
