{ lib, ... }:

{
  imports = [
    ./rover
  ];

  options = {
    nova.networking = {
      wifiIfName = lib.mkOption {
        type = lib.types.str;
        description = "interface name of the wifi";
      };
      ethernetIfName = lib.mkOption {
        type = lib.types.str;
        description = "interface name of the ethernet";
      };
    };
  };

  # ROS 2 middlewares often need to use arbitrary ports.
  # This makes maintaining a firewall difficult.
  networking.firewall.enable = lib.mkDefault false;
}
