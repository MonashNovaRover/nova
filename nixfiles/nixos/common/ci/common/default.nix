{ config, pkgs, lib, ... }:

let
  cfg = config.nova.ci.common;
in
{
  options.nova.ci.common.enable = lib.mkEnableOption "CI common services" // { internal = true; };

  config = lib.mkIf cfg.enable {
    nix.settings = {
      keep-outputs = lib.mkDefault true;
      min-free = lib.mkDefault (01 * 1024 * 1024 * 1024);
      max-free = lib.mkDefault (32 * 1024 * 1024 * 1024);
    };
  };
}
