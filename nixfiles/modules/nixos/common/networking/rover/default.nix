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
    # we are configuring declaratively my with networkd
    networking.networkmanager.unmanaged = [
      netcfg.ethernetInterface
    ];

    systemd.network = {
      enable = true;

      networks = {
        "30-${netcfg.ethernetInterface}" = {
          matchConfig.Name = netcfg.ethernetInterface;
          address = [
            (cfg.ethernetIpAddr + "/23")
          ];
          routes = [
            { Gateway = "10.0.0.1"; }
          ];

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

