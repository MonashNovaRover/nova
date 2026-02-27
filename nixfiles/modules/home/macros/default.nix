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
    programs.bash.initExtra = lib.mkAfter ''
      COMPAL_AUTO_UNMASK=1
      . '${pkgs.complete-alias}/bin/complete_alias'
      complete -F _complete_alias "''${!BASH_ALIASES[@]}"
    '';

    home = {
      shellAliases = lib.mkMerge [
        pkgs.nova.nova.config.shellAliases
        rec {
          # Nix CLI shortcuts
          nova-build = "nix-build ${cfg.nixfileDir}";
          nova-shell = "nix-shell ${cfg.nixfileDir}";
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
          camerasdir = "${nova}/src/ros/cameras2";
          gui = "${nova}/src/ros/nova-gui/nova-gui";
          coms = "${nova}/src/other/coms_utils";

          # Networking 
          jetson = "ssh -Y nvidia@10.0.0.10";
          jetson-wifi = "ssh -Y nvidia@tegra-ubuntu";
          orin = "ssh -Y nova@10.0.0.11";
          orin2 = "ssh -Y nova@10.0.0.12"; # for the other devkit
          orin-devkit-1 = "ssh -Y nova@orin-devkit-1";
          J1 = "ssh -Y nvidia@10.0.2.21";
          J2 = "ssh -Y nvidia@10.0.2.22";
          J3 = "ssh -Y nvidia@10.0.2.23";
          N1 = "ssh -Y nova@10.0.2.11";
          N2 = "ssh -Y nova@10.0.2.12";
          N3 = "ssh -Y nova@10.0.2.13";

          # Application 
          code = "codium";
          urdf-tool = "nix-shell ${cfg.nixfileDir}/modules/home/macros/urdf-tool.nix";

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
          launch-cameras = "~/Builds/master/bin/ros2 launch cameras2 camera_server_launch.py platform:=rover param-dir:='/home/nvidia/nova/src/ros/cameras2/cameras2/params'";
          launch-cameras-all = "${launch-cameras} autostart:=true";
          reolink = "${pkgs.bash}/bin/bash ${../../../scripts/reolink.sh}";

          # Temporary aliases (remove when a better solution has been implemented)
          cameras-legacy = "~/Builds/cameras2legacy/bin/gst-nova-launcher ros2 launch cameras2 camera_server_launch.py platform:=rover param-dir:='/home/nvidia/nova/src/ros/cameras2/cameras2/params' autostart:=true";
          cameras-orin = "~/Builds/cameras2legacy/bin/gst-nova-launcher ros2 launch cameras2 camera_server_launch.py platform:=orin param-dir:='/home/nova/nova/src/ros/cameras2/cameras2/params'";
          cameras-ec = "~/Builds/cameras2legacyarm/bin/gst-nova-launcher ros2 launch cameras2 camera_server_launch.py platform:=orin param-dir:=/home/nova/nova/src/ros/cameras2/cameras2/params payload:=arm";
          nix-enable = "sudo systemctl enable nix-daemon.service";
          nix-start = "sudo systemctl start nix-daemon.service";

          # Setup rover
          zero-arm = "${pkgs.bash}/bin/bash ${../../../scripts/zero-arm.sh}";
          zero-pivots = "${pkgs.bash}/bin/bash ${../../../scripts/zero-pivots.sh}";
          list-blcmds = "more ${cfg.nixfileDir}/doc/blcmd-ids.md";

          # GUI
          gui-serve = "~/Builds/master/bin/gui-serve 5173 && echo http://localhost:5173";

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
          launch-auto-rover = "~/Builds/master/bin/ros2 launch auto_bringup urc.launch.py";
          launch-auto-base = "~/Builds/master/bin/ros2 run nova_utils start_auto.py";
          launch-sim = "~/Builds/master/bin/ros2 launch auto_bringup everything.launch.py";
          launch-auto-hardware = "~/Builds/master/bin/ros2 launch auto_bringup hardware.launch.py";
          launch-auto-software = "~/Builds/master/bin/ros2 launch auto_bringup software.launch.py";
          launch-auto-drive = "~/Builds/master/bin/ros2 launch drive_bringup drive.launch.py auto:=True";
          launch-oak = "~/Builds/master/bin/ros2 launch auto_bringup oak.launch.py";
          launch-localization = "~/Builds/master/bin/ros2 launch auto_bringup localization.launch.py";
          launch-rtabmap = "~/Builds/master/bin/ros2 launch auto_bringup rtabmap.launch.py";
          launch-nav = "~/Builds/master/bin/ros2 launch auto_bringup navigation.launch.py";
          launch-rviz = "~/Builds/master/bin/ros2 launch auto_bringup rviz.launch.py";
          launch-yolo = "~/Builds/master/bin/ros2 launch auto_bringup yolo.launch.py";
          oak-gui = "~/Builds/master/bin/ros2 launch auto_bringup oak-gui.launch.py";

          # GPS
          launch-gps = "~/Builds/master/bin/ros2 launch nova_bringup gps_rover.launch.py gps_params:=/home/nova/nova/src/ros/rover/nova_bringup/params/gps.yaml";
          mast = "ssh nova@10.0.0.150";

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
          predict-shell = "nix-shell ~/nova/src/other/ilmenite_ml"; # please come up with a more descriptive and less generic alias

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
