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

      substituters = [ "https://cuda-maintainers.cachix.org" ];
      trusted-public-keys = [ "cuda-maintainers.cachix.org-1:0dq3bujKpuEPMCX6U4WylrUDZ9JyUG0VpVZa7CNfq5E=" ];
    };

    nova.substituters.ros.enable = true;
  };
}
