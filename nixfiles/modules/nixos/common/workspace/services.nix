{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace.services;
in
{
  options.nova.workspace.services = {
    enable = lib.mkEnableOption "workspace services" // { default = config.nova.workspace.enable; };
    gui = {
      enable = lib.mkEnableOption "GUI services" // { default = true; };
    };
  };

  config = lib.mkIf cfg.enable (lib.mkMerge [
    {
      # Ensure the group exists before defining the user
      users.groups.nova-workspace = { };

      users.users.nova-workspace = {
        isSystemUser = true;
        group = "nova-workspace";
        home = "/var/lib/nova-workspace";
        createHome = true;
      };

      systemd.services = {
        nova-gui = {
          description = "Nova GUI Frontend";
          after = [ "network.target" ];
          wantedBy = [ "multi-user.target" ];
          serviceConfig = {
            Type = "simple";
            User = "nova-workspace";
            Group = "nova-workspace";
            WorkingDirectory = "/var/lib/nova-workspace";
            ExecStart = "${pkgs.bash}/bin/bash -c '
              export ROS_TS_DEFINITIONS=/var/lib/nova-workspace/rosTypes.ts
              ${pkgs.nova-shell}/bin/nova-shell -A pkgs.ros.nova-gui
              cd nova/src/ros/nova-gui/nova-gui
              yarn install
              ln -sf \"$ROS_TS_DEFINITIONS\" src/ros/rosTypes.ts
              yarn dev
            '";
            Restart = "always";
          };
        };

        nova-rosbridge = {
          description = "Nova ROSBridge WebSocket Server";
          after = [ "network.target" ];
          wantedBy = [ "multi-user.target" ];
          serviceConfig = {
            Type = "simple";
            User = "nova-workspace";
            Group = "nova-workspace";
            WorkingDirectory = "/var/lib/nova-workspace";
            ExecStart = "${pkgs.bash}/bin/bash -c 'ros2 launch rosbridge_server rosbridge_websocket_launch.xml'";
            Restart = "always";
          };
        };
      };

      networking.firewall.allowedTCPPorts = [ 80 ];
    }
  ]);
}
