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
      
      cd() {
        echo "$1"
        if [ -d "$1" ]; then
          echo true
        else
          echo false
        fi
        
        if [ ! -d "$1" ] && [ "$1" = "nova" ]; then
          builtin cd "${cfg.sourceDir}/.."
        elif [ ! -d "$1" ] && [ "$1" = "nixfiles" ]; then
          builtin cd "${cfg.nixfileDir}"
        elif [ ! -d "$1" ] && [ "$1" = "rover" ]; then
          builtin cd "${cfg.sourceDir}/ros/rover"
        elif [ ! -d "$1" ] && [ "$1" = "science" ]; then
          builtin cd "${cfg.sourceDir}/ros/rover/science"
        elif [ ! -d "$1" ] && [ "$1" = "gui" ]; then
          builtin cd "${cfg.sourceDir}/ros/nova-gui"
        else
          builtin cd "$1"
        fi
      }
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
            science = "${rover}/science";
            gui = "${nova}/src/ros/nova-gui";

            # Networking aliases
            jetson = "ssh -Y nvidia@10.0.0.10";
            jetson_wifi = "ssh -Y nvidia@tegra-ubuntu";
            orin-devkit-1 = "ssh -Y nova@orin-devkit-1";
            O1 = "ssh -Y nvidia@10.0.2.11";
            O2 = "ssh -Y nvidia@10.0.2.12";
            O3 = "ssh -Y nvidia@10.0.2.13";
            O4 = "ssh -Y nvidia@10.0.2.14";
            J1 = "ssh -Y nvidia@10.0.2.21";
            J2 = "ssh -Y nvidia@10.0.2.22";
            J3 = "ssh -Y nvidia@10.0.2.23";

            # Application aliases
            code = "codium";

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
