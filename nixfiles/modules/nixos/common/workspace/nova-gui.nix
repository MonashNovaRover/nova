{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace.services;
in
{
  options.nova.workspace.services = {
    enable = lib.mkEnableOption "workspace services";
    gui = {
      enable = lib.mkEnableOption "GUI services";
      frontendPackage = lib.mkPackageOption pkgs "frontend resource" {
        default = [ "nova" "nova-gui-frontend" ];
      };
    };
  };

  config = lib.mkIf cfg.enable (lib.mkMerge [
    # General configuration
    {
      users = {
        groups.nova-workspace = { };
        users.nova-workspace = {
          isSystemUser = true;
          group = config.users.groups.nova-workspace.name;
        };
      };

      lib.nova.mkWorkspaceService = { path ? [], script, ... }@args: args // {
        serviceConfig.User = config.users.users.nova-workspace.name;
        serviceConfig.Group = config.users.users.nova-workspace.group;
        serviceConfig.StateDirectory = "nova-workspace";
        serviceConfig.StateDirectoryMode = 0750;
        serviceConfig.LogsDirectory = "nova-workspace";
        path = [
          (config.nova.workspace.package.override {
            interactive = false;
            graphical = false;
            extraPackages = {
              inherit (pkgs.nova.rosPackages.${config.nova.workspace.package.rosDistro})
                ros2cli
                ros2run
                ros2launch;
            };
          })
        ] ++ path;
        script = ''
          export ROS_HOME="$STATE_DIRECTORY"
          export ROS_LOG_DIR="$LOGS_DIRECTORY"
          ${script}
        '';
      };
    }
  ]);

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
