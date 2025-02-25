{ config, pkgs, lib, ... }:

let
  cfg = config.nova.macros;
in
{
  options.nova.macros = {
    enable = lib.mkEnableOption "team shell macros";
    sourceDir = lib.mkOption {
      type = with lib.types; str;
      default = "~/nova/src";
      description = "The source code directory.";
    };
    nixfileDir = lib.mkOption {
      type = with lib.types; str;
      default = "${cfg.sourceDir}/../nixfiles";
      description = "The directory containing the Nix files.";
    };
  };

  config = lib.mkIf cfg.enable {
    programs.bash.initExtra = lib.mkAfter ''
      COMPAL_AUTO_UNMASK=1
      . '${pkgs.complete-alias}/bin/complete_alias'
      complete -F _complete_alias "''${!BASH_ALIASES[@]}"
    '';

    home = {
      shellAliases =
        lib.mkMerge [
          pkgs.nova.nova.config.shellAliases
          rec {
            # Nix CLI shortcuts
            nova-build = "nix-build ${cfg.nixfileDir}";
            nova-shell = "nix-shell ${cfg.nixfileDir}";
            ws-build = "${nova-build} -A pkgs.ros.nova-workspace";
            ws-shell = "${nova-shell} -A pkgs.ros.nova-workspace.env";

            # Directory aliases
            nova = "cd ${cfg.sourceDir}/..";
            nixfiles = "cd ${cfg.nixfileDir}";
            rover = "${nova}/src/ros/rover";
            core = "${nova}/src/ros/rover/core";
            control = "${nova}/src/ros/rover/control";
            electronics = "${nova}/src/ros/rover/electronics";
            elec = electronics;
            visualisation = "${nova}/src/ros/visualisation";
            visualization = visualisation;
            vis = visualisation;
            viz = visualisation;
            science = "${nova}/src/ros/rover/science";
            camerasdir = "${nova}/src/ros/cameras2";
            autonomous = "${nova}/src/ros/rover/autonomous";
            auto = autonomous;
            gui = "${nova}/src/ros/nova-gui";
            tutorials = "${nova}/src/ros/tutorials";
            pic = "${nova}/src/other/pics";
            pics = pic;
            arduino = "${nova}/src/other/arduinos";
            arduinos = arduino;
            ik = "${nova}/src/other/ik_machine";
            coms = "${nova}/src/other/coms_utils";

            # Networking aliases
            jetson = "ssh -Y nvidia@10.0.0.10";
            jetson-wifi = "ssh -Y nvidia@tegra-ubuntu";
            orin = "ssh -Y nova@10.0.0.11";
            orin-devkit-1 = "ssh -Y nova@orin-devkit-1";
            J1 = "ssh -Y nvidia@10.0.2.21";
            J2 = "ssh -Y nvidia@10.0.2.22";
            J3 = "ssh -Y nvidia@10.0.2.23";
            N1 = "ssh -Y nova@10.0.2.11";
            N2 = "ssh -Y nova@10.0.2.12";
            N3 = "ssh -Y nova@10.0.2.13";

            # Application aliases
            code = "codium";
            urdf-tool = "nix-shell ${cfg.nixfileDir}/home/macros/urdf-tool.nix";

            # Stubs to ease migration
            setup = ''echo 'The setup alias is no longer necessary. To try new changes, please use "ws-build" or "nixos-rebuild" instead.' #'';
            check = ''echo 'The check alias is no longer relevant. Use "journalctl -u <service>" instead.' #'';
            stop = ''echo 'The stop alias is no longer relevant. Use "systemctl stop <service>" instead.' #'';
            restart = ''echo 'The restart alias is no longer relevant. Use "systemctl restart <service>" instead.' #'';
            wifi = ''echo 'The wifi alias is no longer relevant. Use "nmtui" or "nmcli" instead.' #'';

            # Nano v Vim
            set_vim = "export EDITOR=vim";
            set_nano = "export EDITOR=nano";

            # ROS Discovery Server
            base_dds_client = "FASTRTPS_DEFAULT_PROFILES_FILE=${./ros_discovery/base_client_configuration.xml}";
            rover_dds_client = "FASTRTPS_DEFAULT_PROFILES_FILE=${./ros_discovery/rover_client_configuration.xml}";
            base_pi_dds_client = "FASTRTPS_DEFAULT_PROFILES_FILE=${./ros_discovery/base_pi_client_configuration.xml}";
            base_dds_super_client = "FASTRTPS_DEFAULT_PROFILES_FILE=${./ros_discovery/base_super_client_configuration.xml}";
            rover_dds_super_client = "FASTRTPS_DEFAULT_PROFILES_FILE=${./ros_discovery/rover_super_client_configuration.xml}";
            base_pi_dds_super_client = "FASTRTPS_DEFAULT_PROFILES_FILE=${./ros_discovery/base_pi_super_client_configuration.xml}";

            # Hydra aliases
            hydra-vomit = "${pkgs.bash}/bin/bash ${../../../scripts/hydra-vomit.sh}";

            # Rover operator aliases
            rover-help = "more ${cfg.nixfileDir}/doc/rover-help.md";
            launch-base = "~/Builds/master/bin/ros2 launch nova_bringup base.launch.py";
            launch-drive = "~/Builds/master/bin/ros2 launch nova_bringup drive.launch.py";
            launch-arm = "~/Builds/master/bin/ros2 launch nova_bringup arm.launch.py";
            launch-ec = "~/Builds/master/bin/ros2 launch nova_bringup ec_rover.launch.py";

            # Rover setup aliases
            zero-arm = "${pkgs.bash}/bin/bash ${../../../scripts/zero-arm.sh}";
            zero-pivots = "${pkgs.bash}/bin/bash ${../../../scripts/zero-pivots.sh}";
            list-blcmds = "more ${cfg.nixfileDir}/doc/blcmd-ids.md";

            # Temporary aliases (remove when a better solution has been implemented)
            cameras-legacy = "~/Builds/cameras2legacy/bin/gst-nova-launcher ros2 launch cameras2 camera_server_launch.py platform:=rover param-dir:='/home/nvidia/nova/src/ros/cameras2/cameras2/params' autostart:=true";
          
            # GUI aliases
            gui-shell = "nova-shell -A pkgs.ros.nova-gui";
            gui-link = "ln -sf \"$ROS_TS_DEFINITIONS\" src/ros/rosTypes.ts";
            gui-rosbridge = "~/Builds/master/bin/ros2 launch rosbridge_server rosbridge_websocket_launch.xml"
          }
        ];

      packages = with pkgs.nova-scripts; [
        can
      ];
    };

    #systemd.user.services.gpsd = {
    #  Unit = {
    #    Description = "Start gpsd on boot";
    #  };
    #  Install = {
    #    WantedBy = [ "default.target" ];
    #  };
    #  Service = {
    #    ExecStart = "${pkgs.writeShellScript "start-gpsd" ''
    #      gpsd -nG /dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0 
    #    ''}";
    #  };
    #};

    nixpkgs.overlays = [
      (self: super: with self; {
        nova-scripts = self.lib.makeScope self.newScope (novaSelf: {
          can = writeShellApplication {
            name = "can";
            runtimeInputs = [ kmod iproute2 ];
            text = builtins.readFile ./can.sh;
          };
        });
      })
    ];
  };
}
