{ config, pkgs, lib, ... }:

let
  cfg = config.nova.ci.slave;
in
{
  options.nova.ci.slave.enable = lib.mkEnableOption "CI slave services";

  config = lib.mkIf cfg.enable {
    nova.ci.common.enable = true;

    assertions = [
      {
        assertion = !config.nova.ci.master.enable;
        message = "The CI master and slave modules cannot be enabled simultaneously.";
      }
    ];
  };
}
