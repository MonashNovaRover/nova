{ lib, config, pkgs, ... }:
let
  cfg = config.nova.networking.rover;
in
{
  options = {
    nova.networking.rover = {
      enable = lib.mkEnableOption "Enable rover networking configuration";
      wifiIfName = lib.mkOption {
        type = lib.types.str;
        description = "interface name of the wifi for the rover";
      };
      ethernetIfName = lib.mkOption {
        type = lib.types.str;
        description = "interface name of the ethernet for the rover";
      };
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

    networking.useDHCP = false;
    networking.networkmanager.enable = lib.mkForce false;
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
        "30-${cfg.wifiIfName}" = {
          matchConfig.Name = cfg.wifiIfName;
          networkConfig.Bridge = "br0";
          #linkConfig.RequiredForOnline = "enslaved";
        };
        "30-${cfg.ethernetIfName}" = {
          matchConfig.Name = cfg.ethernetIfName;
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
    services.hostapd = {
      # there is no dhcp.
      # doesn't seem to be working right yet.
      enable = true;
      radios."${cfg.wifiIfName}" = {
        countryCode = "AU";
        band = "2g";
        channel = 6;
        networks."${cfg.wifiIfName}" = {
          ssid = cfg.hostname + "-hotspot";
          authentication.saePasswords = [
            { passwordFile = ../../../../../secrets/reolink-password.txt; }
          ];
        };
      };
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

