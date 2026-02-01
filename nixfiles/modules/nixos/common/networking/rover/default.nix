{ lib, config, pkgs, ... }:
let
  cfg = config.nova.networking.rover;
in
{
  options = {
    nova.networking.rover = {
      enable = lib.mkEnableOption "Enable rover networking configuration";
      ethernetIfName = lib.mkOption {
        type = lib.types.str;
        description = "interface name of the ethernet for the rover";
      };
      ethernetIpAddr = lib.mkOption {
        type = lib.types.str;
        description = "IP address of the rover over ethernet. must be in 10.0.0.0/23 subnet.";
#TODO: assert ^
      };
    };
  };

  config = lib.mkIf cfg.enable {
    networking = {
      interfaces."${cfg.ethernetIfName}" = {
        ipv4.addresses = [{
          address = cfg.ethernetIpAddr;
          prefixLength = 23;
        }];
      };
      defaultGateway = {
        address = "10.0.0.1";
        interface = cfg.ethernetIfName;
      };
      nameservers = [
        "1.1.1.1" # cloudflare
        "8.8.8.8" # google
        "130.194.1.99" # monash
      ];
    # TODO: enable wifi hotspot
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

