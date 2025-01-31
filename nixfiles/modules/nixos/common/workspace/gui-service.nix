{ config, pkgs, ... }:

{
  systemd.services.nova-gui = {
    description = "Launches nova-gui frontend and backend";
    after = [ "network.target" "roscore.service" ];

    serviceConfig = {
      User = "nova-workspace";
      WorkingDirectory = "/home/nova/nova/src/ros/nova-gui/nova-gui";
      Environment = "HOME=/home/nova";

      ExecStartPre = "${pkgs.nodejs}/bin/yarn install"; 

      ExecStartPost = ''
        ros2 launch rosbridge_server rosbridge_websocket_launch.xml &
      '';

      ExecStart = "${pkgs.nodejs}/bin/yarn dev";

      Restart = "always";
      RestartSec = "3s";  # Restart in case of failure
    };

    wantedBy = [ "multi-user.target" ];
  };
}
