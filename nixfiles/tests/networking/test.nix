{ config, pkgs, lib, ... }:
{
  imports = [
    /etc/nixos/nova/nixfiles/modules/nixos
  ];
  devices.qemu.rover.enable = true;
}
