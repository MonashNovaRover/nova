{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace.services.gui;
in
{
  config = lib.mkIf cfg.enable {
    systemd.services.nova-gui = {
      description = "nova-gui";
      after = [ "network.target" "roscore.service" ];

      serviceConfig = {
        User = config.users.users.nova-workspace.name;
        Group = config.users.users.nova-workspace.group;

        StateDirectory = "nova-workspace";
        StateDirectoryMode = 0750;
        LogsDirectory = "nova-workspace";

        Environment = "HOME=/var/lib/nova-workspace";
        WorkingDirectory = "/var/lib/nova-workspace";

        ExecStartPre = "/run/current-system/sw/bin/nix-shell /var/lib/nova-workspace/nixfiles -A pkgs.ros.nova-gui --run yarn install";

        ExecStart = "${pkgs.yarn}/bin/yarn dev";

        ExecStartPost = ''
          ros2 launch rosbridge_server rosbridge_websocket_launch.xml &
        '';

        Restart = "always";
        RestartSec = "30s";
      };

      wantedBy = [ "multi-user.target" ];
    };
  };
}
