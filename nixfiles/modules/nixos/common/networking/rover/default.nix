{ lib, config, pkgs, ... }:
let
  netcfg = config.nova.networking;
  cfg = config.nova.networking.rover;
in
{
  options = {
    nova.networking.rover = {
      enable = lib.mkEnableOption "Enable rover networking configuration";
      ethernetIpAddr = lib.mkOption {
        type = lib.types.str;
        description = "IP address of the rover over ethernet. must be in 10.0.0.0/23 subnet.";
      };
      hostname = lib.mkOption {
        type = lib.types.str;
        description = "Hostname of the rover";
      };
    };
  };

  config = lib.mkIf cfg.enable {
    # Don't let networkmanager touch interfaces
    # we are configuring decleratively with networkd
    networking.networkmanager.unmanaged = [
      netcfg.ethernetInterface
    ];

    systemd.network = {
      enable = true;
      netdevs = {
        "20-br0" = {
          netdevConfig = {
            Kind = "bridge";
            Name = "br0";
          };
        };
      };

      links = {
        "20-can0" = {
          matchConfig = {
            Path = "platform-c310000.mttcan";
            Driver = "mttcan";
          };
          linkConfig = {
            Name = "can0";
          };
        };

        "20-can1" = {
          matchConfig = {
            Path = "platform-3210000.spi-cs-0";
            Driver = "mcp251xfd";
          };
          linkConfig = {
            Name = "can1";
          };
        };

        "20-can2" = {
          matchConfig = {
            Path = "platform-3230000.spi-cs-0";
            Driver = "mcp251xfd";
          };
          linkConfig = {
            Name = "can2";
          };
        };
      };

      networks = {
        "40-br0" = {
          matchConfig.Name = "br0";
          bridgeConfig = {};
          address = [
            (cfg.ethernetIpAddr + "/23")
          ];
          routes = [
            { Gateway = "10.0.0.1"; }
          ];
        };
        "30-${netcfg.wifiInterface}" = {
          matchConfig.Name = netcfg.wifiInterface;
          networkConfig.Bridge = "br0";
        };
        "30-${netcfg.ethernetInterface}" = {
          matchConfig.Name = netcfg.ethernetInterface;
          networkConfig.Bridge = "br0";

        };
        # CAN
        "60-can0" = {
          matchConfig.Name = "can0";
          canConfig = {
            BitRate = 250000;
            FDMode = false;
          };
        };
        "60-can1" = {
          matchConfig.Name = "can1";
          canConfig = {
            BitRate = 250000;
            FDMode = false;
          };
        };
        "60-can2" = {
          matchConfig.Name = "can2";
          canConfig = {
            BitRate = 250000;
            FDMode = false;
          };
        };
      };
    };

    networking = {
      nameservers = [
        "1.1.1.1" # cloudflare
        "8.8.8.8" # google
        "130.194.1.99" # monash
      ];
      hostName = cfg.hostname;
    };
    assertions = [
      {
        assertion = ((builtins.substring 0 7 cfg.ethernetIpAddr) == "10.0.0." )
          || ( (builtins.substring 0 7 cfg.ethernetIpAddr) == "10.0.1.");
        message = "The rover ip address must be in 10.0.0.0/23 subnet.";
      }
    ];
  };
}

