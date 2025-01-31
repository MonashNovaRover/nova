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
        gui-frontend = {
          wantedBy = [ "multi-user.target" ];
          requires = [];
          after = [];
          workingDirectory = "/home/nova/nova/src/ros/nova-gui/nova-gui";  # Set the working directory directly
          path = with pkgs; [
            (writeShellScriptBin "start-nova-gui-frontend" ''
              #!/bin/sh
              yarn install
              yarn dev
            '')
            (writeShellScriptBin "start-rosbridge" ''
              #!/bin/sh
              ros2 launch rosbridge_server rosbridge_websocket_launch.xml
            '')
          ];
          script = ''
            # Start the frontend
            /bin/sh $PATH_TO_START_NOVA_GUI_FRONTEND &
            # Start the rosbridge server
            /bin/sh $PATH_TO_START_ROSBRIDGE &
            wait
          '';
          serviceConfig.User = "nova-workspace";  # Ensure this is the correct user
          serviceConfig.Group = "nova-workspace";  # Ensure this is the correct group
          serviceConfig.AmbientCapabilities = [ "CAP_NET_BIND_SERVICE" ];
        };
      };

      networking.firewall.allowedTCPPorts = [ 80 ];
    })
  ]);
}
