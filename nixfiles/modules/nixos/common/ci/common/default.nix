{ config, pkgs, lib, ... }:

let
  cfg = config.nova.ci.common;
in
{
  options.nova.ci.common.enable = lib.mkEnableOption "CI common services" // { internal = true; };

  config = lib.mkIf cfg.enable {
    nix.settings = {
      auto-optimise-store = lib.mkDefault true;
      min-free = lib.mkDefault (01 * 1024 * 1024 * 1024);
      max-free = lib.mkDefault (32 * 1024 * 1024 * 1024);
      keep-outputs = lib.mkDefault true;

      substituters = [
        "https://ros.cachix.org"
        "https://cache.nixos-cuda.org"
        "https://cache.flox.dev"
      ];
      trusted-public-keys = [
        "ros.cachix.org-1:dSyZxI8geDCJrwgvCOHDoAfOm5sV1wCPjBkKL+38Rvo="
        "cache.nixos-cuda.org:74DUi4Ye579gUqzH4ziL9IyiJBlDpMRn9MBN8oNan9M="
        "flox-cache-public-1:7F4OyH7ZCnFhcze3fJdfyXYLQw/aV7GEed86nQ7IsOs="
      ];
    };

    nova.substituters.ros.enable = true;
  };
}
