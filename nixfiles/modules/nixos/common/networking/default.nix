{ lib, ... }:
{
  options.nova.networking = {
    wifiInterface = lib.mkOption {
      type = lib.types.str;
      description = "Interface name of this computer's wifi.";
    };
    ethernetInterface = lib.mkOption {
      type = lib.types.str;
      description = "Interface name of this computer's etherent.";
    };
    secondaryEthernetInterface = lib.mkOption {
      type = lib.types.str;
      description = "Interface name of this computer's secondary etherent.";
    };
  };

  imports = [
    ./rover
    ./mast
  ];

  config = {
    # ROS 2 middlewares often need to use arbitrary ports.
    # This makes maintaining a firewall difficult.
    networking.firewall.enable = lib.mkDefault false;

    # For easy wifi configuration
    networking.networkmanager.enable = lib.mkDefault true;
  };
}
