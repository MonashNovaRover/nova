{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace.services;
in
{
  options.nova.workspace.services = {
    enable = lib.mkEnableOption "workspace services";
    gui = {
      enable = lib.mkEnableOption "GUI services" // { default = true; };
      frontendPackage = lib.mkPackageOption pkgs "frontend resource" {
        default = [ "nova" "nova-gui-frontend" ];
      };
    };
  };

  # Ensure the user and group for nova-workspace are set up correctly
  config = lib.mkIf cfg.enable (lib.mkMerge [
    {
      users = {
        groups.nova-workspace = {};  # Create the group
        users.nova-workspace = {
          isSystemUser = true;
          group = config.users.groups.nova-workspace.name;
          shell = pkgs.zsh;  # Or whichever shell is used
          home = "/home/nova";
        };
      };

      # Define the nova-workspace service with appropriate permissions and environment
      systemd.services.nova-gui = {
        description = "Launches nova-gui frontend and backend";
        after = [ "network.target" "roscore.service" ];

        serviceConfig = {
          User = "nova-workspace";  # Run the service as nova-workspace user
          Group = "nova-workspace";  # Use nova-workspace group

          WorkingDirectory = "/home/nova/nova/src/ros/nova-gui/nova-gui";
          Environment = "HOME=/home/nova";

          # Run yarn install using the nova-workspace environment
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
  ];
}
