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
   nova.services.ptpd = {
     enable = true;
     interfaceName = "prp0";
   };

   nova.networking.prp = {
     enable = true;
     networkmanager= false;
     address = builtins.substring 5 12 cfg.ethernetIpAddr;
   };

    systemd.network = {
      enable = true;

      # rename canable to usbcan0
      links = {
        "70-usbcan" = {
          matchConfig = {
            Property = "ID_BUS=usb";
            Type = "can";
          };
          linkConfig = {
            Name = "usbcan0";
          };
        };
      };

      networks = {
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

