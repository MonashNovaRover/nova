{
  rosPackages = pkgs: with pkgs; {
    nova-electronics = callPackage ./nix/packages/electronics { };
    nova-science = callPackage ./nix/packages/science { };
    nova-blcmd-hardware = callPackage ./nix/packages/blcmd-hardware { };
    nova-costmap-2d = callPackage ./nix/packages/nova-costmap-2d { };
    nova-behavior-tree = callPackage ./nix/packages/nova-behavior-tree { };
    nova-object-localisation = callPackage ./nix/packages/nova-object-localisation { };
    nova-teleop-drive-joy = callPackage ./nix/packages/teleop-drive-joy { };
    nova-teleop-arm-joy = callPackage ./nix/packages/teleop-arm-joy { };
    nova-pointcloud-filter = callPackage ./nix/packages/nova-pointcloud-filter { };
    nova-drive = callPackage ./nix/packages/drive { };
    nova-drive-interfaces = callPackage ./nix/packages/drive-interfaces { };
    nova-blcmd-interfaces = callPackage ./nix/packages/blcmd-interfaces { };
    nova-blcmd-utils = callPackage ./nix/packages/blcmd-utils { };
    nova-arm-interfaces = callPackage ./nix/packages/arm-interfaces { };
    nova-arm = callPackage ./nix/packages/arm { };
    nova-input-interfaces = callPackage ./nix/packages/input-interfaces { };
    nova-inputs = callPackage ./nix/packages/inputs { };
    nova-cmd-interfaces = callPackage ./nix/packages/cmd-interfaces { };
    nova-cmd-utils = callPackage ./nix/packages/cmd-utils { };
    nova-gimbal-cam = callPackage ./nix/packages/gimbal-cam { };
    nova-bringup = callPackage ./nix/packages/nova-bringup { };
    nova-auto-bringup = callPackage ./nix/packages/auto-bringup { };
    nova-arm-bringup = callPackage ./nix/packages/arm-bringup { };
    nova-interfaces = callPackage ./nix/packages/nova-interfaces { };
    nova-rover-description = callPackage ./nix/packages/rover-description { };
    nova-gazebo = callPackage ./nix/packages/nova-gazebo { };
    nova-python-control = callPackage ./nix/packages/python-control { };
    nova-python-control-old = callPackage ./nix/packages/python-control-old { };
    nova-excavation-construction = callPackage ./nix/packages/excavation-construction { };
    nova-detection-overlay = callPackage ./nix/packages/nova-detection-overlay { };
    nova-bt-navigators = callPackage ./nix/packages/nova-bt-navigators { };
    nova-auto-interfaces = callPackage ./nix/packages/nova-auto-interfaces { };
    nova-utils = callPackage ./nix/packages/nova-utils { };
    lattice-primitive-generator = callPackage ./nix/packages/lattice-primitive-generator { };
    ublox-dgnss-custom = callPackage ./nix/packages/ublox-dgnss { };

    # diff drive, pivot drive, strafe, 
  } // import ./nix/packages/controllers { inherit pkgs; };

  #pythonPackages = pythonPackages: with pythonPackages; {
    
  #};

  shellAliases = {
    # Launching aliases
    base = "ros2 launch nova_bringup base.launch.py";
    drive = "ros2 launch nova_bringup drive.launch.py";
    arm = "ros2 launch nova_bringup arm.launch.py";
    sci = "ros2 launch nova_bringup urc_science.launch.py";
    auto_drive = "ros2 launch auto_bringup control.launch.py";
    localisation = "ros2 launch auto_bringup localization.launch.py";
    localization = "ros2 launch auto_bringup localization.launch.py";
    urdf = "ros2 launch nova_bringup urdf.launch.py";
    rosbridge = "ros2 launch rosbridge_server rosbridge_websocket_launch.xml";

    # Service aliases
    arm_config_info = "ros2 service call control/arm_config_info arm_interfaces/srv/ArmConfigInfo";
    arm_reset_control_pose = "ros2 service call control/arm_reset_control_pose std_srvs/srv/Trigger";

    # Autonomous aliases
    auto_bag = "ros2 bag record /T265/pose /depth_camera/d435_1/cloud /autonomous/occupancy_grid /object_detector/markers /ar_tracker/tags goal_manager/confirmed_targets -s mcap -b 500000000";
    start_mapping = "ros2 param set /mapper do_mapping False";
    stop_mapping = "ros2 param set /mapper do_mapping True";
  };
}
