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
        # Defaults
        export RMW_IMPLEMENTATION="rmw_cyclonedds_cpp"
        export COMP="ARCh"
        ln -sfn "$HOME/Builds/master" "$HOME/Builds/active"

        # Environment variables
        ## ROS2 DDS configuration
        use_fastdds() {
          export RMW_IMPLEMENTATION="rmw_fastrtps_cpp"
          mkdir -p "$HOME/.config/nova"
          echo 'export RMW_IMPLEMENTATION=rmw_fastrtps_cpp' > "$HOME/.config/nova/ros_dds"
          echo "Set RMW_IMPLEMENTATION to rmw_fastrtps_cpp and wrote to ~/.config/nova/ros_dds"
        }
        use_cyclonedds() {
          export RMW_IMPLEMENTATION="rmw_cyclonedds_cpp"
          mkdir -p "$HOME/.config/nova"
          echo 'export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp' > "$HOME/.config/nova/ros_dds"
          echo "Set RMW_IMPLEMENTATION to rmw_cyclonedds_cpp and wrote to ~/.config/nova/ros_dds"
        }

        ## Comp selection
        set_arch() {
          export COMP="ARCh"
          mkdir -p "$HOME/.config/nova"
          echo 'export COMP=ARCh' > "$HOME/.config/nova/comp"
          echo "Set COMP to ARCh and wrote to ~/.config/nova/comp"
        }
        set_urc() {
          export COMP="URC"
          mkdir -p "$HOME/.config/nova"
          echo 'export COMP=URC' > "$HOME/.config/nova/comp"
          echo "Set COMP to URC and wrote to ~/.config/nova/comp"
        }

        # Set active build path
        set_active() {
          local buildPath="$1"
          if [ -z "$buildPath" ]; then
            echo "usage: set_active <build_path>"
            return 2
          fi

          local runtimeDir="/run/user/$UID"

          ln -sfn "$buildPath" "$HOME/Builds/active"
          mkdir -p "$runtimeDir/nova"
          printf 'ln -sfn %q %q\n' "$buildPath" "$HOME/Builds/active" > "$runtimeDir/nova/active_build"
          echo "Set active build to ''${buildPath/#$HOME/\~} and wrote to $runtimeDir/nova/active_build"
        }

        # Source the ROS2 DDS configuration if it exists
        if [ -f $HOME/.config/nova/ros_dds ]; then
          . $HOME/.config/nova/ros_dds
        fi

        # Source the COMP environment variable if it exists
        if [ -f $HOME/.config/nova/comp ]; then
          . $HOME/.config/nova/comp
        fi

        # Source the active build configuration if it exists
        if [ -f "/run/user/$UID/nova/active_build" ]; then
          . "/run/user/$UID/nova/active_build"
        fi

        # Calculate box width based on longest content line (in subshell to auto-cleanup)
        # Only show in interactive shells to avoid breaking scp/rsync/etc
        [[ $- == *i* ]] && (
          # Display /home/nova/path as ~/path
          active_build="$(readlink "$HOME/Builds/active")"
          active_build="''${active_build/#$HOME/\~}"
          
          # Labels and values
          title="Nova Shell Status (Ctrl+L to clear)"
          help_label="(?) " help_value="nova_sh_help"
          rmw_label="RMW: " rmw_value="''${RMW_IMPLEMENTATION:-not set}"
          comp_label="COMP: " comp_value="''${COMP:-not set}"
          build_label="Active Build: " build_value="$active_build"
          
          # Find maximum width
          max_len=''${#title}
          for val in "''${rmw_label}''${rmw_value}" "''${comp_label}''${comp_value}" "''${build_label}''${build_value}"; do
            [ $((''${#val} + 2 )) -gt $max_len ] && max_len=$((''${#val} + 2 ))
          done
          [ $((''${#help_label} + ''${#help_value})) -gt $max_len ] && max_len=$((''${#help_label} + ''${#help_value}))
          
          # Build and display box
          # Box width is the length of the longest line, plus padding (1 space before and after)
          width=$((max_len + 2))
          border=$(printf '%.0s─' $(seq 1 $width))

          # Colours
          cyan='\033[1;36m'
          magenta='\033[1;35m'
          yellow='\033[1;33m'
          end='\033[0m'
          
          echo   "┌$border┐"
          printf "│ $cyan%-$((width-1))s$end│\n" "$title"
          printf "│ $magenta%s$end%s%-$((width-1-''${#help_label}-''${#help_value}))s│\n" "$help_label" "$help_value" ""
          printf "│   $yellow%s$end%s%-$((width-3-''${#rmw_label}-''${#rmw_value}))s│\n" "$rmw_label" "$rmw_value" ""
          printf "│   $yellow%s$end%s%-$((width-3-''${#comp_label}-''${#comp_value}))s│\n" "$comp_label" "$comp_value" ""
          printf "│   $yellow%s$end%s%-$((width-3-''${#build_label}-''${#build_value}))s│\n" "$build_label" "$build_value" ""
          echo   "└$border┘"
          echo   ""
        )

        # Help text for nova shell
        nova_sh_help() {
          local yellow='\033[1;33m' end='\033[0m'
          printf "''${yellow}RMW:''${end} Set with use_(fast|cyclone)dds aliases.\n"
          echo   "The RMW_IMPLEMENTATION environment variable. Used by ROS2 to select the DDS implementation."
          echo   "Config is written to ~/.config/nova/ros_dds"
          echo   ""
          printf "''${yellow}COMP:''${end} Set with set_(arch|urc) aliases.\n"
          echo   "The COMP environment variable. Used by auto in launch files to choose between params and configurations."
          echo   "Config is written to ~/.config/nova/comp"
          echo   ""
          printf "''${yellow}Active Build:''${end} Set with set_active <build_path> function.\n"
          echo   "~/Builds/active -> <build_path> symlink. Allows aliases to point to a build other than master. Currently only used by auto."
          echo   "Config is written to /run/user/''${UID}/nova/active_build"
          echo   ""
          echo   "The files written to are sourced on shell startup."
          echo   ""
        }
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

          # Hydra 
          hydra-vomit = "${pkgs.bash}/bin/bash ${../../../scripts/hydra-vomit.sh}";

          # Launch rover or payloads
          rover-help = "more ${cfg.nixfileDir}/doc/rover-help.md";
          launch-teleop = "echo 'DEPRECATED - Please use launch-teleop-drive instead for drive teleop'";
          launch-teleop-drive = "~/Builds/master/bin/ros2 launch teleop_drive_joy teleop.launch.py";
          launch-drive = "~/Builds/master/bin/ros2 launch drive_bringup drive.launch.py";
          launch-base = "~/Builds/master/bin/ros2 launch nova_bringup base.launch.py";
          launch-old-drive = "~/Builds/master/bin/ros2 launch nova_bringup old_drive.launch.py";
          launch-teleop-arm = "~/Builds/master/bin/ros2 launch teleop_arm teleop.launch.py";
          launch-arm = "~/Builds/master/bin/ros2 launch arm_bringup control.launch.py";
          launch-old-arm = "~/Builds/master/bin/ros2 launch nova_bringup arm.launch.py";
          launch-ec = "~/Builds/master/bin/ros2 launch nova_bringup ec_rover.launch.py";
          launch-teleop-ec = "~/Builds/master/bin/ros2 launch teleop_ec teleop.launch.py";
          launch-teleop-science = "~/Builds/master/bin/ros2 launch teleop_science teleop.launch.py";
          launch-science-arc = "~/Builds/master/bin/ros2 launch science_bringup arc.launch.py";
          launch-science-urc = "~/Builds/master/bin/ros2 launch science_bringup urc_old.launch.py";
          launch-theta-orin = "sudo LANG=C ~/Builds/master/bin/ros2 run science urc_theta_360_cam.py";

          # Cameras
          reolink = "${pkgs.bash}/bin/bash ${../../../scripts/reolink.sh}";

          cameras3 = "~/Builds/master/bin/ros2 launch cameras cameras.launch.py";
          cameras2 = "~/Builds/master/bin/ros2 launch cameras2 camera_server_launch.py platform:=orin param-dir:=/home/nova/nova/src/ros/cameras2/cameras2/params";
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
          gui-serve = "~/Builds/master/bin/gui-serve 5173 && echo http://localhost:5173";

          gui-dev-shell = "nova-shell -A pkgs.ros.nova-gui-dev-shell";
          gui-shell = "nova-shell -A pkgs.ros.nova-gui";
          gui-link = "ln -sf \"$ROS_TS_DEFINITIONS\" ~/nova/src/ros/nova-gui/nova-gui/src/ros/rosTypes.ts";
          gui-rosbridge = "~/Builds/master/bin/ros2 launch rosbridge_server rosbridge_websocket_launch.xml";
          gui-run = "yarn --cwd ~/nova/src/ros/nova-gui/nova-gui dev";
          gui-yarn = "yarn --cwd ~/nova/src/ros/nova-gui/nova-gui";

          # Tile server
          tileserver = "~/Builds/master/bin/mbtileserver -p 8080 --missing-image-tile-404 -d ~/maps";

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
          launch-everything = "~/Builds/active/bin/ros2 launch auto_bringup everything.launch.py";
          launch-auto-drive = "~/Builds/active/bin/ros2 launch drive_bringup drive.launch.py auto:=True";
          launch-realsense = "~/Builds/active/bin/ros2 launch auto_bringup realsense.launch.py";
          launch-localization = "~/Builds/active/bin/ros2 launch auto_bringup localization.launch.py";
          launch-lidar = "~/Builds/active/bin/ros2 launch auto_bringup lidar.launch.py";
          launch-navigation = "~/Builds/active/bin/ros2 launch auto_bringup navigation.launch.py";
          launch-rviz = "~/Builds/active/bin/ros2 launch auto_bringup rviz.launch.py";
          launch-auto-urdf = "~/Builds/active/bin/ros2 launch auto_bringup urdf.launch.py";
          launch-yolo = "~/Builds/active/bin/ros2 launch auto_bringup yolo.launch.py";
          start-arch = "~/Builds/active/bin/ros2 run nova_utils start_auto_arch.py";
          start-urc = "~/Builds/active/bin/ros2 run auto_start start_auto_urc.py";
          scp-pcd = "scp nova@10.0.0.50:/home/nova/output.pcd.zip ~/ && unzip ~/output.pcd.zip";

          # GPS
          launch-gps = "~/Builds/master/bin/ros2 launch nova_bringup gps_rover.launch.py gps_params:=/home/nova/nova/src/ros/rover/nova_bringup/params/gps.yaml";
          mast = "ssh -C nova@10.0.0.150";

          # Master build binaries
          mros2 = "~/Builds/master/bin/ros2";
          mrviz2 = "~/Builds/master/bin/rviz2";
          mrviz = "~/Builds/master/bin/rviz2";
          mxacro = "~/Builds/master/bin/xacro ${cfg.sourceDir}/ros/rover/rover_description/banksia/urdf/rover.urdf.xacro";
          mrqt = "~/Builds/master/bin/rqt";

          # Arm
          launch-typing = "~/Builds/master/bin/ros2 launch arm_bringup typing.launch.py";
          launch-arm-control = "~/Builds/master/bin/ros2 launch arm_bringup control.launch.py arm:=False old_arm:=True";
          launch-path-control = "~/Builds/master/bin/ros2 launch arm_bringup path.control.launch.py arm:=False old_arm:=True";
          launch-arm-urdf = "~/Builds/master/bin/ros2 launch arm_bringup urdf.launch.py arm:=False old_arm:=True auto_camera:=False";
          launch-arm-teleop = "~/Builds/master/bin/ros2 launch teleop_arm teleop.launch.py";
          run-arm-teleop = "~/Builds/master/bin/ros2 run teleop_arm_joy teleop_arm_joy_node";
          run-arm-teleop-xbox = "~/Builds/master/bin/ros2 run teleop_arm_joy teleop_arm_joy_node --ros-args --params-file ${cfg.sourceDir}/ros/rover/teleop_arm_joy/config/old.xbox.config.yaml";
          run-joy = "~/Builds/master/bin/ros2 run joy joy_node";

          # Science
          predict-shell = "nom-shell ~/nova/src/other/ilmenite_ml"; # please come up with a more descriptive and less generic alias

          # ros2_control
          controllers-list = "~/Builds/master/bin/ros2 control list_controllers";
          controllers-set = "~/Builds/master/bin/ros2 control set_controller_state";
          activate-path = "~/Builds/master/bin/ros2 control set_controller_state nova_path_planner active";
          deactivate-path = "~/Builds/master/bin/ros2 control set_controller_state nova_path_planner inactive";

          # Ducket
          ducket-pos = "cansend can0 0C1#7000";
          ducket-neg = "cansend can0 0C1#9000";
          ducket = "cansend can0 0C1#7000";
          ducketn = "cansend can0 0C1#9000";

          # can
          lscan = "for bus in $(ip link show type vcan | cut -d : -f2 | grep -v -e link -e alias | tr -d ' ') $(ip link show type can | cut -d : -f2 | grep -v -e link -e alias | tr -d ' '); do echo $bus; udevadm info /sys/class/net/$bus | grep DEVPATH; done";

          can-sleuth = "can_sleuth";
          can_sleuth = "~/Builds/master/bin/can_sleuth";

          # use this as `can_viewer can0` for example to get "-c can0"
          can-viewer = "can_viewer";
          can_viewer = "~/Builds/master/bin/can_viewer -i socketcan -c";
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
