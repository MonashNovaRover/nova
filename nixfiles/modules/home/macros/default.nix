{
  config,
  pkgs,
  lib,
  ...
}:

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
    programs.bash = {
      bashrcExtra = lib.mkAfter ''
        # Nova ENV variable defaults
        export RMW_IMPLEMENTATION="rmw_fastrtps_cpp"
        export COMP="ARCh"

        # set_master <path>
        set_master() {
          local buildPath="$1"
          if [ -z "$buildPath" ]; then
            echo "usage: set_master <path>"
            return 2
          fi

          local runtimeDir="/run/user/$UID"

          export PSEUDO_MASTER="$buildPath"
          mkdir -p "$runtimeDir/nova"
          echo "export PSEUDO_MASTER='$buildPath'" > "$runtimeDir/nova/pseudo_master"
          echo "Set PSEUDO_MASTER to ''${buildPath/#$HOME/\~} and wrote to $runtimeDir/nova/pseudo_master"
        }

        # Source the ROS2 DDS configuration if it exists
        if [ -f ~/.config/nova/ros_dds ]; then
          . ~/.config/nova/ros_dds
        fi

        # Source the COMP environment variable if it exists
        if [ -f ~/.config/nova/comp ]; then
          . ~/.config/nova/comp
        fi

        # Source the PSEUDO_MASTER environment variable if it exists
        if [ -f "/run/user/$UID/nova/pseudo_master" ]; then
          . "/run/user/$UID/nova/pseudo_master"
        fi

        # Display as ~/path instead of /home/user/path
        PSEUDO_MASTER_DISPLAY="''${PSEUDO_MASTER:-not set}"
        if [ "$PSEUDO_MASTER_DISPLAY" != "not set" ]; then
          PSEUDO_MASTER_DISPLAY="''${PSEUDO_MASTER_DISPLAY/#$HOME/\~}"
        fi
        
        # title width = 34
        # entry width = 32
        echo   "┌───────────────────────────────────┐"
        printf "│ %s│\n"   "$(printf "\033[1;36m%-34s\033[0m" "Nova ENV Status (Ctrl+L to clear)")"
        printf "│   %s│\n" "$(printf "\033[1;33m%s\033[0m%-27s" "RMW: " "''${RMW_IMPLEMENTATION:-not set}")"
        printf "│   %s│\n" "$(printf "\033[1;33m%s\033[0m%-26s" "COMP: " "''${COMP:-not set}")"
        printf "│   %s│\n" "$(printf "\033[1;33m%s\033[0m%-17s" "PSEUDO_MASTER: " "$PSEUDO_MASTER_DISPLAY")"
        echo   "└───────────────────────────────────┘"
        echo   ""

        unset PSEUDO_MASTER_DISPLAY
      '';

      initExtra = lib.mkAfter ''
        COMPAL_AUTO_UNMASK=1
        . '${pkgs.complete-alias}/bin/complete_alias'
        complete -F _complete_alias "''${!BASH_ALIASES[@]}"
      '';
    };

    home = {
      shellAliases = lib.mkMerge [
        pkgs.nova.nova.config.shellAliases
        (
          let
            masterBuild = "\${PSEUDO_MASTER:-~/Builds/master}";
          in
          rec {
            # System
            off = "sudo poweroff";
            kfc = "can stop can0";

            # Nix CLI shortcuts
            nova-build = "nom-build ${cfg.nixfileDir}";
            nova-shell = "nom-shell ${cfg.nixfileDir}";
            ws-build = "${nova-build} -A pkgs.ros.nova-workspace";
            ws-shell = "${nova-shell} -A pkgs.ros.nova-workspace.env";

            cameras-build = "${nova-build} -A misc.cameras2-legacy.launcher -o ~/Builds/cameras2legacy";

            # Directory aliases
            nova = "cd ${cfg.sourceDir}/..";
            nixfiles = "cd ${cfg.nixfileDir}";
            rover = "${nova}/src/ros/rover";
            arm = "${nova}/src/ros/rover/arm";
            autonomous = "${nova}/src/ros/rover/auto";
            auto = autonomous;
            chassis = "${nova}/src/ros/rover/chassis";
            science = "${nova}/src/ros/rover/science";
            camerasdir = "${nova}/src/ros/cameras";
            gui = "${nova}/src/ros/nova-gui/nova-gui";
            coms = "${nova}/src/other/coms_utils";

            # Environment variables
            ## ROS2 DDS Configuration
            use_fastdds = "export RMW_IMPLEMENTATION=rmw_fastrtps_cpp;
                           mkdir -p ~/.config/nova;
                           echo 'export RMW_IMPLEMENTATION=rmw_fastrtps_cpp' > ~/.config/nova/ros_dds
                           echo \"Set RMW_IMPLEMENTATION to rmw_fastrtps_cpp and wrote to ~/.config/nova/ros_dds\"";
            use_cyclonedds = "export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp;
                              mkdir -p ~/.config/nova;
                              echo 'export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp' > ~/.config/nova/ros_dds
                              echo \"Set RMW_IMPLEMENTATION to rmw_cyclonedds_cpp and wrote to ~/.config/nova/ros_dds\"";
            
            ## Comp Selection (for auto)
            set_arch = "export COMP=ARCh;
                        mkdir -p ~/.config/nova;
                        echo 'export COMP=ARCh' > ~/.config/nova/comp
                        echo \"Set COMP to ARCh and wrote to ~/.config/nova/comp\"";
            set_urc = "export COMP=URC;
                       mkdir -p ~/.config/nova;
                       echo 'export COMP=URC' > ~/.config/nova/comp
                       echo \"Set COMP to URC and wrote to ~/.config/nova/comp\"";
            
            # Networking 
            jetson = "ssh -C -Y nvidia@10.0.0.10";
            jetson-wifi = "ssh -C -Y nvidia@tegra-ubuntu";
            orin = "ssh -C -Y nova@10.0.0.11";
            orin2 = "ssh -C -Y nova@10.0.0.12"; # for the other devkit
            pi5 = "ssh -C -Y nova@10.0.0.50";
            orin-devkit-1 = "ssh -C -Y nova@orin-devkit-1";
            J1 = "ssh -C -Y nvidia@10.0.2.21";
            J2 = "ssh -C -Y nvidia@10.0.2.22";
            J3 = "ssh -C -Y nvidia@10.0.2.23";
            N1 = "ssh -C -Y nova@10.0.2.11";
            N2 = "ssh -C -Y nova@10.0.2.12";
            N3 = "ssh -C -Y nova@10.0.2.13";

            # Application 
            urdf-tool = "nom-shell ${cfg.nixfileDir}/modules/home/macros/urdf-tool.nix";

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

            # Hydra 
            hydra-vomit = "${pkgs.bash}/bin/bash ${../../../scripts/hydra-vomit.sh}";

            # Launch rover or payloads
            rover-help = "more ${cfg.nixfileDir}/doc/rover-help.md";
            launch-teleop = "echo 'DEPRECATED - Please use launch-teleop-drive instead for drive teleop'";
            launch-teleop-drive = "${masterBuild}/bin/ros2 launch teleop_drive_joy teleop.launch.py";
            launch-drive = "${masterBuild}/bin/ros2 launch drive_bringup drive.launch.py";
            launch-base = "${masterBuild}/bin/ros2 launch nova_bringup base.launch.py";
            launch-old-drive = "${masterBuild}/bin/ros2 launch nova_bringup old_drive.launch.py";
            launch-teleop-arm = "${masterBuild}/bin/ros2 launch teleop_arm teleop.launch.py";
            launch-arm = "${masterBuild}/bin/ros2 launch arm_bringup control.launch.py";
            launch-old-arm = "${masterBuild}/bin/ros2 launch nova_bringup arm.launch.py";
            launch-ec = "${masterBuild}/bin/ros2 launch nova_bringup ec_rover.launch.py";
            launch-teleop-ec = "${masterBuild}/bin/ros2 launch teleop_ec teleop.launch.py";
            launch-teleop-science = "${masterBuild}/bin/ros2 launch teleop_science teleop.launch.py";
            launch-science-arc = "${masterBuild}/bin/ros2 launch science_bringup arc.launch.py";
            launch-science-urc = "${masterBuild}/bin/ros2 launch science_bringup urc_old.launch.py";
            launch-theta-orin = "sudo LANG=C ${masterBuild}/bin/ros2 run science urc_theta_360_cam.py";

            # Cameras
            reolink = "${pkgs.bash}/bin/bash ${../../../scripts/reolink.sh}";

            cameras3 = "${masterBuild}/bin/ros2 launch cameras cameras.launch.py";
            cameras2 = "${masterBuild}/bin/ros2 launch cameras2 camera_server_launch.py platform:=orin param-dir:=/home/nova/nova/src/ros/cameras2/cameras2/params";
            cameras2-legacy = "~/Builds/cameras2legacy/bin/gst-nova-launcher ros2 launch cameras2 camera_server_launch.py platform:=orin param-dir:='/home/nova/nova/src/ros/cameras2/cameras2/params'";
            cameras-orin ="echo 'DEPRECATED - Please use cameras instead for cameras operation, or cameras2-legacy for old camera stack'";
            cameras = "${cameras3}";
            nix-enable = "sudo systemctl enable nix-daemon.service";
            nix-start = "sudo systemctl start nix-daemon.service";

            # Setup rover
            zero-arm = "${pkgs.bash}/bin/bash ${../../../scripts/zero-arm.sh}";
            zero-pivots = "${pkgs.bash}/bin/bash ${../../../scripts/zero-pivots.sh}";
            list-blcmds = "more ${cfg.nixfileDir}/doc/blcmd-ids.md";
            mast-up="cansend can0 0E2#1000; sleep 5.2; cansend can0 0E2#0000";

            # GUI
            gui-serve = "${masterBuild}/bin/gui-serve 5173 && echo http://localhost:5173";

            gui-shell = "nova-shell -A pkgs.ros.nova-gui";
            gui-link = "ln -sf \"$ROS_TS_DEFINITIONS\" ~/nova/src/ros/nova-gui/nova-gui/src/ros/rosTypes.ts";
            gui-rosbridge = "${masterBuild}/bin/ros2 launch rosbridge_server rosbridge_websocket_launch.xml";
            gui-run = "yarn --cwd ~/nova/src/ros/nova-gui/nova-gui dev";
            gui-yarn = "yarn --cwd ~/nova/src/ros/nova-gui/nova-gui";

            # Tile server
            tileserver = "${masterBuild}/bin/mbtileserver -p 8080 --missing-image-tile-404 -d ~/maps";

            # LEDs
            leds-red = "cansend can0 095#0100";
            leds-green = "cansend can0 095#0200";
            leds-blue = "cansend can0 095#0300";
            leds-pink = "cansend can0 096#";
            leds-100 = "cansend can0 091#8000";
            leds-75 = "cansend can0 091#6000";
            leds-50 = "cansend can0 091#4000";
            leds-off = "cansend can0 091#0000";

            # Bonus
            cop-mode-on = "${pkgs.bash}/bin/bash ${../../../scripts/cop-mode.sh} on";
            cop-mode-off = "${pkgs.bash}/bin/bash ${../../../scripts/cop-mode.sh} off";

            # Auto 
            launch-auto-rover = "${masterBuild}/bin/ros2 launch auto_bringup urc.launch.py";
            launch-auto-base = "${masterBuild}/bin/ros2 run nova_utils start_auto.py";
            launch-sim = "${masterBuild}/bin/ros2 launch auto_bringup everything.launch.py";
            launch-auto-hardware = "${masterBuild}/bin/ros2 launch auto_bringup hardware.launch.py";
            launch-auto-software = "${masterBuild}/bin/ros2 launch auto_bringup software.launch.py";
            launch-auto-drive = "${masterBuild}/bin/ros2 launch drive_bringup drive.launch.py auto:=True";
            launch-oak = "${masterBuild}/bin/ros2 launch auto_bringup oak.launch.py";
            launch-realsense = "${masterBuild}/bin/ros2 launch auto_bringup realsense.launch.py";
            launch-localization = "${masterBuild}/bin/ros2 launch auto_bringup localization.launch.py";
            launch-lidar = "${masterBuild}/bin/ros2 launch auto_bringup lidar.launch.py";
            launch-rtabmap = "${masterBuild}/bin/ros2 launch auto_bringup rtabmap.launch.py";
            launch-nav = "${masterBuild}/bin/ros2 launch auto_bringup navigation.launch.py";
            launch-rviz = "${masterBuild}/bin/ros2 launch auto_bringup rviz.launch.py";
            launch-auto-urdf = "${masterBuild}/bin/ros2 launch auto_bringup urdf.launch.py";
            launch-yolo = "${masterBuild}/bin/ros2 launch auto_bringup yolo.launch.py";
            oak-gui = "${masterBuild}/bin/ros2 launch auto_bringup oak-gui.launch.py";
            scp-pcd = "scp nova@10.0.0.50:/home/nova/output.pcd.zip ~/ && unzip ~/output.pcd.zip";
            start-arch = "${masterBuild}/bin/ros2 run nova_utils start_auto_arch.py";

            # GPS
            launch-gps = "${masterBuild}/bin/ros2 launch nova_bringup gps_rover.launch.py gps_params:=/home/nova/nova/src/ros/rover/nova_bringup/params/gps.yaml";
            mast = "ssh -C nova@10.0.0.150";

            # Master build binaries
            mros2 = "${masterBuild}/bin/ros2";
            mrviz2 = "${masterBuild}/bin/rviz2";
            mrviz = "${masterBuild}/bin/rviz2";
            mxacro = "${masterBuild}/bin/xacro ${cfg.sourceDir}/ros/rover/rover_description/banksia/urdf/rover.urdf.xacro";
            mrqt = "${masterBuild}/bin/rqt";

            # Arm
            launch-typing = "${masterBuild}/bin/ros2 launch arm_bringup typing.launch.py";
            launch-arm-control = "${masterBuild}/bin/ros2 launch arm_bringup control.launch.py arm:=False old_arm:=True";
            launch-path-control = "${masterBuild}/bin/ros2 launch arm_bringup path.control.launch.py arm:=False old_arm:=True";
            launch-arm-urdf = "${masterBuild}/bin/ros2 launch arm_bringup urdf.launch.py arm:=False old_arm:=True auto_camera:=False";
            launch-arm-teleop = "${masterBuild}/bin/ros2 launch teleop_arm teleop.launch.py";
            run-arm-teleop = "${masterBuild}/bin/ros2 run teleop_arm_joy teleop_arm_joy_node";
            run-arm-teleop-xbox = "${masterBuild}/bin/ros2 run teleop_arm_joy teleop_arm_joy_node --ros-args --params-file ${cfg.sourceDir}/ros/rover/teleop_arm_joy/config/old.xbox.config.yaml";
            run-joy = "${masterBuild}/bin/ros2 run joy joy_node";

            # Science
            predict-shell = "nom-shell ~/nova/src/other/ilmenite_ml"; # please come up with a more descriptive and less generic alias

            # ros2_control
            controllers-list = "${masterBuild}/bin/ros2 control list_controllers";
            controllers-set = "${masterBuild}/bin/ros2 control set_controller_state";
            activate-path = "${masterBuild}/bin/ros2 control set_controller_state nova_path_planner active";
            deactivate-path = "${masterBuild}/bin/ros2 control set_controller_state nova_path_planner inactive";

            # Ducket
            ducket-pos = "cansend can0 0C1#7000";
            ducket-neg = "cansend can0 0C1#9000";
            ducket = "cansend can0 0C1#7000";
            ducketn = "cansend can0 0C1#9000";

            # can
            lscan = "for bus in $(ip link show type vcan | cut -d : -f2 | grep -v -e link -e alias | tr -d ' ') $(ip link show type can | cut -d : -f2 | grep -v -e link -e alias | tr -d ' '); do echo $bus; udevadm info /sys/class/net/$bus | grep DEVPATH; done";

            can-sleuth = "can_sleuth";
            can_sleuth = "${masterBuild}/bin/can_sleuth";

            # use this as `can_viewer can0` for example to get "-c can0"
            can-viewer = "can_viewer";
            can_viewer = "${masterBuild}/bin/can_viewer -i socketcan -c";
          }
        )
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
      (
        self: super: with self; {
          nova-scripts = self.lib.makeScope self.newScope (novaSelf: {
            can = writeShellApplication {
              name = "can";
              runtimeInputs = [
                kmod
                iproute2
              ];
              text = builtins.readFile ./can.sh;
            };
          });
        }
      )
    ];
  };
}
