{ config, pkgs, lib, ... }:

let
  cfg = config.nova.ci.common;
in
{
  options.nova.ci.common.enable = lib.mkEnableOption "CI common services" // { internal = true; };

  config = lib.mkIf cfg.enable {
    nix.settings = lib.mkDefault {
      keep-outputs = true;
    };
  };
}
