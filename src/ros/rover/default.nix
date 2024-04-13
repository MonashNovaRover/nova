{
  rosPackages = pkgs: with pkgs; {
    nova-core = callPackage ./nix/packages/core { };
    nova-control = callPackage ./nix/packages/control { };
    nova-autonomous = callPackage ./nix/packages/autonomous { };
    nova-electronics = callPackage ./nix/packages/electronics { };
    nova-science = callPackage ./nix/packages/science { };
    nova-blcmd-hardware = callPackage ./nix/packages/blcmd-hardware { };
    nova-costmap-2d = callPackage ./nix/packages/nova-costmap-2d { };
    nova-behavior-tree = callPackage ./nix/packages/nova-behavior-tree { };
    nova-cube-localisation = callPackage ./nix/packages/nova-cube-localisation { };
    nova-teleop-drive-joy = callPackage ./nix/packages/teleop-drive-joy { };
    nova-pointcloud-filter = callPackage ./nix/packages/nova-pointcloud-filter { };
    nova-drive= callPackage ./nix/packages/drive { };
    nova-drive-msgs = callPackage ./nix/packages/drive-msgs { };
    nova-blcmd-interfaces = callPackage ./nix/packages/blcmd-interfaces { };
    nova-blcmd-utils = callPackage ./nix/packages/blcmd-interfaces { };
    nova-arm-interfaces = callPackage ./nix/packages/arm-interfaces { };
    nova-arm = callPackage ./nix/packages/arm { };
    nova-input-msgs = callPackage ./nix/packages/input-msgs { };
    nova-inputs = callPackage ./nix/packages/inputs { };  
    # diff drive, pivot drive, strafe, 
  } // import ./nix/packages/controllers { inherit pkgs; };

  pythonPackages = pythonPackages: with pythonPackages; {
    ultralytics = pythonPackages.callPackage ./nix/packages/misc/ultralytics { };
  };

  shellAliases = {
    # Launching aliases
    base = "ros2 launch core base.launch.py";
    drive = "ros2 launch core drive.launch.py";
    arm = "ros2 launch core arm.launch.py";
    arm_spoof = "ros2 launch core arm_spoof.launch.py";
    sci = "ros2 launch core science.launch.py";
    unity = "ros2 launch core visualisation.launch.py";
    auto_drive = "ros2 launch core auto_drive.launch.py";
    localisation = "ros2 launch core localisation.launch.py";
    urdf = "ros2 launch core urdf.launch.py";
    launch_viz = "ros2 launch core viz.launch.py";
    launch_vis = "ros2 launch core viz.launch.py";
    rosbridge = "ros2 launch rosbridge_server rosbridge_websocket_launch.xml";
    ec_jetson = "ros2 launch core ec_jetson_launch.py";
    ec_base = "ros2 launch core base.launch.py";

    # Service aliases
    arm_config_info = "ros2 service call control/arm_config_info arm_interfaces/srv/ArmConfigInfo";
    arm_reset_control_pose = "ros2 service call control/arm_reset_control_pose std_srvs/srv/Trigger";

    # Autonomous aliases
    auto_bag = "ros2 bag record /T265/pose /depth_camera/d435_1/cloud /autonomous/occupancy_grid /object_detector/markers /ar_tracker/tags goal_manager/confirmed_targets -s mcap -b 500000000";
    start_mapping = "ros2 param set /mapper do_mapping False";
    stop_mapping = "ros2 param set /mapper do_mapping True";
  };
}
