{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace.services;
in
{
  options.nova.workspace.services = {
    enable = lib.mkEnableOption "workspace services" // { default = config.nova.workspace.enable; };
    gui = {
      enable = lib.mkEnableOption "GUI services" // { default = true; };
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
        gui-backend = config.lib.nova.mkWorkspaceService {
          description = "Nova ROSBridge WebSocket Server";
          script = "ros2 launch rosbridge_server rosbridge_websocket_launch.xml";
        };

        gui-frontend = {
          wantedBy = [ "multi-user.target" ];
          requires = [ "gui-backend.service" ];
          after = [ "gui-backend.service" ];
          path = with pkgs; [
            (nova.nova-gui-frontend-server.override {
              nova-gui-frontend = cfg.gui.frontendPackage;
            })
            nodejs
            yarn
          ];
          script = ''
            cd nova/src/ros/nova-gui/nova-gui
            yarn install
            ln -sf "$ROS_TS_DEFINITIONS" src/ros/rosTypes.ts
            yarn dev
          '';
          serviceConfig.DynamicUser = true;
          serviceConfig.AmbientCapabilities = [ "CAP_NET_BIND_SERVICE" ];
        };
      };

      networking.firewall.allowedTCPPorts = [ 80 ];
    })
  ]);
}
