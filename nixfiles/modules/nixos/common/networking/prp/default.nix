{ lib, config, pkgs, ... }:
let
  netcfg = config.nova.networking;
  cfg = config.nova.networking.prp;
in {
  options = {
    nova.networking.prp = {
      enable = lib.mkEnableOption "Enable parallel redundancy protocol.";
      # the redundant path doesn't seem to work with network manager :(
      networkmanager = lib.mkEnableOption "DOESN'T WORK. Use network manager to control the final redundant connection";
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
    networking.networkmanager = {
      unmanaged = [
        netcfg.ethernetInterface
        netcfg.secondaryEthernetInterface
        "br0"
        "vlan5"
        "vlan9"
        #"vxlan0"
        (lib.mkIf (!cfg.networkmanager) "prp0")
      ];
      ensureProfiles.profiles = lib.mkIf cfg.networkmanager {
        prp-shared = {
          connection = {
            id = "prp-shared";
            interface-name = "prp0";
            autoconnect=false;
            type="hsr";
          };
          ipv4 = {
            method = "shared";
            address1 = "10.0.0.1/23";
          };
          hsr = {
            port1 = netcfg.ethernetInterface;
            port2 = "br0";
            prp = true;
          };
        };
        prp-normal = {
          connection = {
            id = "prp-normal";
            interface-name = "prp0";
            autoconnect=true;
            type="hsr";
          };
          ipv4 = {
            method = "manual";
            address1 = ("10.0." + cfg.address + "/23");
            gateway = "10.0.0.1";
            route-metric = 2000; # prefer wifi
          };
          hsr = {
            port1 = netcfg.ethernetInterface;
            port2 = "br0";
            prp = true;
          };
        };
      };
    };

    systemd.network = {
      links = {
        "70-usbeth" = {
          matchConfig = {
            Property = "ID_BUS=usb";
          };
          linkConfig = {
            Name = "usbeth0";
          };
        };
      };
    };

    systemd.network = {
      enable = true;
      netdevs = {
        "20-br0" = {
          # bridge to ensure there is an underlying device
          # for prp0 to be created on even if the secondary
          # ethernet was not plugged in yet
          netdevConfig = {
            Kind = "bridge";
            Name = "br0";
          };
        };
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
        };
        "20-br0" = {
          matchConfig.Name = "br0";
          linkConfig = {
            ActivationPolicy = "always-up";
          };
        };
        "80-prp0" = lib.mkIf (!cfg.networkmanager) {
          matchConfig.Name = "prp0";
          address = [
            ("10.0." + cfg.address + "/23")
          ];
          routes = [
            {
              Gateway = "10.0.0.1";
              Metric = 2000;
            }
          ];
        };
        "30-PRP-B-${netcfg.secondaryEthernetInterface}" = {
          matchConfig.Name = netcfg.secondaryEthernetInterface;
          networkConfig = {
            VLAN = "vlan9";
            Bridge = "br0";
          };
        };
        "30-PRP-A-${netcfg.ethernetInterface}" = {
          matchConfig.Name = netcfg.ethernetInterface;
          networkConfig = {
            VLAN = "vlan5";
          };
        };
      };
    };
    # NixOS doesn't let you configure HSR netdevs :/
    environment.etc."systemd/network/30-prp0.netdev" = lib.mkIf (!cfg.networkmanager) {
        text = ''
        [NetDev]
        Kind = hsr
        Name = prp0
        [HSR]
        Protocol = prp
        Ports = ${netcfg.ethernetInterface}
        Ports = br0
      '';
    };

    assertions = [
      {
        assertion = ((builtins.substring 0 2 cfg.address) == "0." )
          || ( (builtins.substring 0 2 cfg.address) == "1.");
        message = "address must be 0.XXX or 1.XXX";
      }
    ];
  };
}

