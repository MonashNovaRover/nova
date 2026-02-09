{ lib, ... }:

{
  imports = [
    ./rover
  ];

  # ROS 2 middlewares often need to use arbitrary ports.
  # This makes maintaining a firewall difficult.
  networking.firewall.enable = lib.mkDefault false;
}
