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
    };
  };

  config = lib.mkIf cfg.enable {
    systemd.services.ptpd = {
      enable = true;
      after = [ "network.target" ];
      wantedBy = [ "default.target" ];
      description = "Precision Time Protocol Daemon";
      serviceConfig = {
        Type = "simple";
        ExecStart = ''${pkgs.nova.ptpd}/bin/ptpd2 -C -m -i ${cfg.interfaceName}'';
      };
    };
  };
}

