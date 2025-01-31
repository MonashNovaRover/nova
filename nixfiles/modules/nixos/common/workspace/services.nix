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
          group = "nova-workspace";
          home = "/home/nova-workspace";  # Ensure the home directory is correct
          createHome = true;
        };
      };

      lib.nova.mkWorkspaceService = { path ? [ ], script, ... }@args: args // {
        serviceConfig.User = "nova-workspace";
        serviceConfig.Group = "nova-workspace";
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
      systemd.services.gui-frontend = {
        wantedBy = [ "multi-user.target" ];
        requires = [ "gui-backend.service" ];
        after = [ "gui-backend.service" ];
        serviceConfig = {
          Type = "simple";
          User = "nova-workspace";  # Ensuring the service runs as nova-workspace
          Group = "nova-workspace";  # Ensuring the service runs in the nova-workspace group
          WorkingDirectory = "/home/nova-workspace/nova/src/ros/nova-gui/nova-gui";
          Restart = "always";
          Environment = [
            "ROS_TS_DEFINITIONS=/home/nova-workspace/nova/src/ros/nova-gui/nova-gui/src/ros/rosTypes.ts"
          ];
        };

        script = ''
          if [ ! -d "/home/nova-workspace/nova/src/ros/nova-gui/nova-gui" ]; then
            echo "Creating the nova-gui directory..."
            mkdir -p /home/nova-workspace/nova/src/ros/nova-gui/nova-gui
          fi

          if [ ! -L "src/ros/rosTypes.ts" ]; then
            echo "Creating symlink for rosTypes.ts..."
            ln -sf "$ROS_TS_DEFINITIONS" src/ros/rosTypes.ts
          fi

          if [ ! -d "node_modules" ]; then
            echo "Installing dependencies..."
            yarn install
          fi
          yarn dev
        '';
      };

      networking.firewall.allowedTCPPorts = [ 80 ];  # Allow traffic on port 80 for the frontend
    })
  ]);
}
