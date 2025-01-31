{ config, pkgs, lib, ... }:

{
  systemd.services.nova-gui = {
    description = "Launches nova-gui frontend and backend";
    after = [ "network.target" "roscore.service" ];

    serviceConfig = {
      User = "nova-workspace";
      WorkingDirectory = "/home/nova/nova/src/ros/nova-gui/nova-gui";
      Environment = "HOME=/home/nova";

      # Use the full path to nix-shell
      ExecStartPre = "/run/current-system/sw/bin/nix-shell /home/nova/nova/nixfiles -A pkgs.ros.nova-gui --run yarn install";

      # Launch the backend (rosbridge)
      ExecStartPost = ''
        ros2 launch rosbridge_server rosbridge_websocket_launch.xml &
      '';

      # Start the frontend (yarn dev)
      ExecStart = "${pkgs.yarn}/bin/yarn dev";

      Restart = "always";
      RestartSec = "3s";  # Restart in case of failure
    };

    wantedBy = [ "multi-user.target" ];
  };
}
