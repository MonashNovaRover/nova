{ lib, config, pkgs, ... }:
let
  netcfg = config.nova.networking;
  cfg = config.nova.networking.prp;
in {
  options = {
    nova.networking.prp = {
      enable = lib.mkEnableOption "Enable parallel redundancy protocol.";
      address = lib.mkOption {
        type = lib.types.str;
        description = ''
          last part of the ip addresses for this computer in form X.YYY where X is 0 or 1 and YYY is 0-255.
          if you put 1.10, you would get these three IP addresses:

          10.0.1.10/23: redundant so it transmits over both 900MHz and 5GHz. Devices without PRP enabled can still be reached from this subnet.
          10.5.1.10/23: only transmit over 5GHz radio to other devices with PRP enabled
          10.9.1.10/23: only transmit over 900MHz radio to other devices with PRP enabled
          '';
      };
    };
  };

  config = lib.mkIf cfg.enable {
    # Don't let networkmanager touch interfaces
    # we are configuring decleratively with networkd
    networking.networkmanager.unmanaged = [
      netcfg.ethernetInterface
      netcfg.secondaryEthernetInterface
      "vlan5"
      "vlan9"
      "vxlan0"
      "prp0"
    ];

    systemd.network = {
      enable = true;
      netdevs = {
        "20-vlan9" = {
          # VLAN for 900MHz only traffic
          netdevConfig = {
            Kind = "vlan";
            Name = "vlan9";
          };
          vlanConfig = {
            Id = 9;
          };
        };
        "20-vlan5" = {
          # VLAN for 5/2.4Ghz only traffic
          netdevConfig = {
            Kind = "vlan";
            Name = "vlan5";
          };
          vlanConfig = {
            Id = 5;
          };
        };
        "20-vxlan0" = {
          # VXLAN to ensure the rover switch never sees prp0's mac address on the 900MHz side
          netdevConfig = {
            Kind = "vxlan";
            Name = "vxlan0";
          };
          vxlanConfig = {
            VNI = 7; # Kevin's choice
            Group = "239.1.1.7";
            DestinationPort = 4789;
          };
        };
      };

      networks = {
        "40-vlan5" = {
          matchConfig.Name = "vlan5";
          address = [
            ("10.5." + cfg.address + "/23")
          ];
        };
        "40-vlan9" = {
          matchConfig.Name = "vlan9";
          address = [
            ("10.9." + cfg.address + "/23")
          ];
          networkConfig = {
            VXLAN = "vxlan0";
          };
        };
        "20-vxlan0" = {
          matchConfig.Name = "vxlan0";
          linkConfig = {
            ActivationPolicy = "always-up";
          };
        };
        "40-prp0" = {
          matchConfig.Name = "prp0";
          address = [
            ("10.0." + cfg.address + "/23")
          ];
          routes = [
            { Gateway = "10.0.0.1"; }
          ];
        };
        "30-PRP-B-${netcfg.secondaryEthernetInterface}" = lib.mkIf (netcfg.secondaryEthernetInterface != netcfg.ethernetInterface) {
          matchConfig.Name = netcfg.secondaryEthernetInterface;
          networkConfig = {
            VLAN = "vlan9";
          };
        };
        "30-PRP-A-${netcfg.ethernetInterface}" = lib.mkIf (netcfg.secondaryEthernetInterface != netcfg.ethernetInterface) {
          matchConfig.Name = netcfg.ethernetInterface;
          networkConfig = {
            VLAN = "vlan5";
          };
        };
        "30-PRP-AB-${netcfg.ethernetInterface}" = lib.mkIf (netcfg.secondaryEthernetInterface == netcfg.ethernetInterface) {
          matchConfig.Name = netcfg.ethernetInterface;
          networkConfig = {
            VLAN = "vlan5\nVLAN=vlan9";
          };
        };
      };
    };
    # NixOS doesn't let you configure HSR netdevs :/
    # VXLAN Must be first slave! the prp0 must have the same mac address as vxlan0 or all packets set to prp0 over vxlan will be sent as multicast not unicast
    environment.etc."systemd/network/30-prp0.netdev".text = ''
      [NetDev]
      Kind = hsr
      Name = prp0
      [HSR]
      Protocol = prp
      Ports = vxlan0
      Ports = ${netcfg.ethernetInterface}
    '';
      #Ports = ${netcfg.secondaryEthernetInterface}

    assertions = [
      {
        assertion = ((builtins.substring 0 2 cfg.address) == "0." )
          || ( (builtins.substring 0 2 cfg.address) == "1.");
        message = "address must be 0.XXX or 1.XXX";
      }
    ];
  };
}

