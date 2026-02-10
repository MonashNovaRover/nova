{ lib, config, pkgs, ... }:
let
  cfg = config.nova.services.nova-gui;
in
{
  options = {
    nova.services.nova-gui = {
      enable = lib.mkEnableOption "Enable the Nova GUI";
      port = lib.mkOption {
        type = lib.types.ints.u16;
        description = "tcp port the gui should be served on";
        default = 80;
      };
    };
  };

  config = lib.mkIf cfg.enable {
    systemd.services.rosbridge = {
      enable = true;
      after = [ "network.target" ];
      wantedBy = [ "nova-gui.service" ];
      description = "Nova GUI";
      serviceConfig = {
        # Ros nodes can't run as root as they won't be able to communicate with ros nodes running as nova
        User = "nova";
        Type = "simple";
        ExecStart = ''${pkgs.nova.ros.nova-workspace}/bin/ros2 launch rosbridge_server rosbridge_websocket_launch.xml'';
      };
    };
    systemd.services.nova-gui = {
      enable = true;
      after = [ "network.target" ];
      wantedBy = [ "default.target" ];
      description = "Nova GUI";
      serviceConfig = {
        Type = "simple";
        ExecStart = ''${pkgs.nova.ros.nova-gui}/bin/serve-gui ${toString cfg.port}'';
      };
    };
  };
}

