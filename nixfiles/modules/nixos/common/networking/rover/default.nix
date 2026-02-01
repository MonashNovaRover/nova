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
      hostName = cfg.hostname;
    };
    services.create_ap = {
      enable = true;
      settings = {
        CHANNEL = "6"; # ARCh Rule 3.12.4.1 only allows this channel without restrictions
	GATEWAY = "10.0.0.1";
	NO_DNS = "1";
	#NO_DNSMASQ=0
	SHARE_METHOD = "bridge"; # People say this is bad for ros, let's see if it really is.
	FREQ_BAND = "2.4";
	WIFI_IFACE = cfg.wifiIfName;
	INTERNET_IFACE = cfg.ethernetIfName;
	SSID = cfg.hostname + "-hotspot";
	PASSPHRASE = builtins.readFile ../../../../../secrets/reolink-password.txt;
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

