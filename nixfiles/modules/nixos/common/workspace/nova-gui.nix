{ config, pkgs, lib, ... }:

{
  systemd.services.nova-gui = {
    description = "Launches nova-gui frontend and backend";
    after = [ "network.target" "roscore.service" ];

    serviceConfig = {
      User = "nova-workspace";
      Group = "nova-workspace";  # Ensuring the user is in the correct group
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

  # Ensure correct user and group with appropriate permissions
  users = {
    groups.nova-workspace = {};

    users.nova-workspace = {
      isSystemUser = true;
      group = "nova-workspace";
      home = "/home/nova";
      shell = pkgs.zsh;  # Default shell (can be adjusted as needed)
    };
  };

  # Ensure the correct directory permissions for the nova-workspace user
  environment.etc."passwd".text = ''
    nova-workspace:x:1001:1001:nova-workspace:/home/nova:/run/current-system/sw/bin/bash
  '';
  systemd.tmpfiles.rules = [
    "d /home/nova/nova/src/ros/nova-gui/nova-gui 0770 nova-workspace nova-workspace -"
  ];
}
