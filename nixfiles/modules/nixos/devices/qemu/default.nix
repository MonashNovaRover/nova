{ config, pkgs, lib, ... }:
let
  cfg = config.devices.qemu;
in
{
  imports = [
    #/etc/nixos/nova/nixfiles/modules/nixos
    <nixpkgs/nixos/modules/virtualisation/qemu-vm.nix>
    ./rover.nix
  ];

  options.devices.qemu.enable = lib.mkEnableOption "configuration for qemu";

  config = lib.mkIf cfg.enable {
    boot.loader.systemd-boot.enable = true;
    boot.loader.efi.canTouchEfiVariables = true;


    nova.profile = "shared";
    nova.substituters.nova.password = builtins.readFile /etc/nixos/nova/nixfiles/secrets/hydra-password.txt;
    home-manager.sharedModules = [{
      home.stateVersion = "23.05";
    }];

    system.stateVersion = "24.05";
  };
}
