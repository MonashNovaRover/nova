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
      after = [ "sys-subsystem-net-devices-${cfg.interfaceName}.device" ];
      bindsTo = [ "sys-subsystem-net-devices-${cfg.interfaceName}.device" ];

      wantedBy = [ "multi-user.target" ];
      description = "Precision Time Protocol Daemon";
      startLimitIntervalSec = 0;

      script = ''
        while true; do
          state=$(cat /sys/class/net/${cfg.interfaceName}/operstate 2>/dev/null || echo down)
          echo "Waiting for network interface ${cfg.interfaceName} to be up..."
          if [ "$state" = "up" ]; then
            exec ${pkgs.nova.ptpd}/bin/ptpd2 -C -m -i ${cfg.interfaceName} --ptpengine:priority1=${toString cfg.priority}
          fi
          sleep 2
        done
      '';

      serviceConfig = {
        Type = "simple";
        Restart = "on-failure";
        RestartSec = 2;
      };
    };
  };
}

