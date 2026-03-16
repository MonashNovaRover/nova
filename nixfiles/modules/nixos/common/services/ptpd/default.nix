{ lib, config, pkgs, ... }:
let
  cfg = config.nova.services.ptpd;
in
{
  options = {
    nova.services.ptpd = {
      enable = lib.mkEnableOption "Enable Precision Time Protocol Daemon for LAN time synchronisation";
      interfaceName = lib.mkOption {
        type = lib.types.str;
        description = "network interface name that PTP should run on";
      };
      priority = lib.mkOption {
        type = lib.types.int;
        description = "priority of the PTP daemon (lower is higher priority, default is 128, max is 248)";
        default = 128;
      };
    };
  };

  config = lib.mkIf cfg.enable {
    systemd.services.ptpd = {
      enable = true;
      after = [ "network-online.target" ];
      wantedBy = [ "multi-user.target" ];
      description = "Precision Time Protocol Daemon";
      serviceConfig = {
        Type = "simple";
        ExecStart = ''${pkgs.nova.ptpd}/bin/ptpd2 -C -m -i ${cfg.interfaceName} --ptpengine:priority1=${toString cfg.priority}'';
      };
    };
  };
}

