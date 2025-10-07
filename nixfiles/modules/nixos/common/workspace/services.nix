{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace.services;
in
{
  options.nova.workspace.services = {
    enable = lib.mkEnableOption "workspace services" // { default = config.nova.workspace.enable; };
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
  ]);
}
