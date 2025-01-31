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
          path = with pkgs; [ (writeShellScriptBin "clear" "") ];
          script = ''
            source /opt/ros/${config.nova.workspace.package.rosDistro}/setup.bash
            ros2 launch rosbridge_server rosbridge_websocket_launch.xml || exit 1
          '';
        };

        gui-frontend = {
          wantedBy = [ "multi-user.target" ];
          requires = [ "gui-backend.service" ];
          after = [ "gui-backend.service" ];
          path = with pkgs; [
            (nova.nova-gui-frontend-server.override {
              nova-gui-frontend = cfg.gui.frontendPackage;
            })
          ];
          serviceConfig = {
            DynamicUser = true;
            AmbientCapabilities = [ "CAP_NET_BIND_SERVICE" ];
            User = "nova-workspace";
            Group = "nova-workspace";
            WorkingDirectory = "/home/nova/nova/src/ros/nova-gui/nova-gui";  # Ensure full path
            Restart = "always";
          };
          script = ''
            echo "Starting gui-frontend service..."
            # Ensure directory exists and symlink is set up
            if [ ! -d "/home/nova/nova/src/ros/nova-gui/nova-gui" ]; then
              echo "Directory does not exist: /home/nova/nova/src/ros/nova-gui/nova-gui"
              exit 1
            fi

            echo "Setting up symlink for rosTypes.ts..."
            if [ ! -L "src/ros/rosTypes.ts" ]; then
              ln -sf "$ROS_TS_DEFINITIONS" src/ros/rosTypes.ts
            fi

            # Install dependencies if not installed
            if [ ! -d "node_modules" ]; then
              echo "Installing dependencies..."
              yarn install || exit 1
            fi

            # Start the frontend server
            echo "Starting frontend server..."
            yarn dev || exit 1
          '';
        };
      };

      networking.firewall.allowedTCPPorts = [ 80 ];
    })
  ]);
}
