{ lib, config, pkgs, ... }:
let
  cfg = config.nova.networking;
in
{
  options = {
    nova.networking.mast = {
      enable = lib.mkEnableOption "Enable mast networking configuration";
      ip = lib.mkOption {
        type = lib.types.str;
        description = "IP address of the mast. Must be in 10.0.0.0/23 subnet.";
      };
    };
  };

  config = lib.mkIf cfg.mast.enable {

    # Don't let networkmanager touch interfaces
    # we are configuring decleratively with networkd
    networking.networkmanager.unmanaged = [
      cfg.ethernetInterface
    ];

    systemd.network = {
      enable = true;

      networks = {
        "40-${cfg.ethernetInterface}" = {
          matchConfig.Name = cfg.ethernetInterface;
          address = [
            (cfg.mast.ip + "/23")
          ];
          routes = [
            { Gateway = "10.0.0.1"; }
          ];
        };
      };
    };

    networking = {
      nameservers = [
        "1.1.1.1" # cloudflare
        "8.8.8.8" # google
        "130.194.1.99" # monash
      ];
    };
    assertions = [
      {
        assertion = ((builtins.substring 0 7 cfg.mast.ip) == "10.0.0." )
          || ( (builtins.substring 0 7 cfg.mast.ip) == "10.0.1.");
        message = "The mast ip address must be in 10.0.0.0/23 subnet.";
      }
    ];
  };
}

