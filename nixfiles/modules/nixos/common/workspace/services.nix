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

      lib.nova.mkWorkspaceService = { path ? [ ], script, ... }@args: args // {
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

    # GUI services
    (lib.mkIf cfg.gui.enable {
      systemd.services = {
        # Backend service
        gui-backend = {
          wantedBy = [ "multi-user.target" ];
          script = ''
            # Start the rosbridge server
            ros2 launch rosbridge_server rosbridge_websocket_launch.xml
          '';
          serviceConfig.User = "nova-workspace";
          serviceConfig.Group = "nova-workspace";
        };

        # Frontend service
        gui-frontend = {
          wantedBy = [ "multi-user.target" ];
          requires = [ "gui-backend.service" ];
          after = [ "gui-backend.service" ];
          path = with pkgs; [
            (writeShellScriptBin "start-nova-gui-frontend" ''
              #!/bin/sh
              cd /home/nova/nova/src/ros/nova-gui/nova-gui
              yarn install
              yarn dev
            '')
          ];
          script = "start-nova-gui-frontend";
          serviceConfig.User = "nova-workspace";
          serviceConfig.Group = "nova-workspace";
          serviceConfig.AmbientCapabilities = [ "CAP_NET_BIND_SERVICE" ];
        };
      };

      networking.firewall.allowedTCPPorts = [ 80 ];
    })
  ]);
}
