{ config, pkgs, lib, ... }:

let
  # Extract the correct directory paths based on the configuration options
  cfg = config.nova.macros;
  nixfileDir = lib.strings.concatStringsSep "/" [ cfg.sourceDir ".." "nixfiles" ];
in
{
  systemd.services.nova-gui = {
    description = "Launches nova-gui frontend and backend";
    after = [ "network.target" "roscore.service" ];

    serviceConfig = {
      User = "nova-workspace";
      WorkingDirectory = "/home/nova/nova/src/ros/nova-gui/nova-gui";
      Environment = "HOME=/home/nova";

      # Ensure the correct environment is set up with nix-shell
      ExecStartPre = ''
        # Use nix-shell to set up the environment and run yarn install
        nix-shell ${nixfileDir} -A pkgs.ros.nova-gui --run "${pkgs.nodejs}/bin/yarn install"
      '';

      # Launch the backend (rosbridge)
      ExecStartPost = ''
        ros2 launch rosbridge_server rosbridge_websocket_launch.xml &
      '';

      # Start the frontend (yarn dev)
      ExecStart = "${pkgs.nodejs}/bin/yarn dev";

      Restart = "always";
      RestartSec = "3s";  # Restart in case of failure
    };

    wantedBy = [ "multi-user.target" ];
  };
}
