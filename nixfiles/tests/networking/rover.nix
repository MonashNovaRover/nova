{ config, pkgs, lib, ... }:
let
  cfg = config.devices.qemu.rover;
  true_ = "${pkgs.coreutils-full}/bin/true";
in
{
  options.devices.qemu.rover.enable = lib.mkEnableOption "virtual rover configuration for qemu";

  config = lib.mkIf cfg.enable {
    devices.qemu.enable = true;

    virtualisation.qemu = {
      networkingOptions = lib.mkForce [
        # disable the scripts so it doesn't try to create a bridge and complain about /etc/qemu/bridge.conf being missing
        # vm's eth0 is host's tap0 (for 5GHz)
        "-netdev tap,id=net0,ifname=tap0,script=${true_}"
        "-device virtio-net-pci,netdev=net0"
        # vm's eth1 is host's tap1 (for 900MHz)
        "-netdev tap,id=net1,ifname=tap1,script=${true_}"
        "-device virtio-net-pci,netdev=net1"
      ];
      options = [

        # CAN BUS
        "-object can-bus,id=canbus0"
        #"-object can-host-socketcan,if=can0,canbus=canbus0-bus,id=canbus0-socketcan"
        "-device kvaser_pci,canbus=canbus0"
        "-object can-bus,id=canbus1"
        #"-object can-host-socketcan,if=can1,canbus=canbus1-bus,id=canbus1-socketcan"
        "-device kvaser_pci,canbus=canbus1"
        "-object can-bus,id=canbus2"
        #"-object can-host-socketcan,if=can2,canbus=canbus2-bus,id=canbus2-socketcan"
        "-device kvaser_pci,canbus=canbus2"

        # remove this later
        "-nographic"
      ];
    };

    nova.desktop.enable = false;

    nova.mocking = {
      cameras = {
        # TODO: fix serials
        enable = true;
        count = 1;
        specs = {
          width = 800;
          height = 600;
        };
      };
    };

    networking.hostName = "rover";
     nova.networking.rover = {
      enable = true;
      hostname = "rover";
      wifiIfName = "none";
      ethernetIfName = "eth0";
      ethernetIpAddr = "10.0.0.10";
    };
  };
}
