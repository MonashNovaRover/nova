{
  rosPackages = pkgs: with pkgs; {
    nova-electronics = callPackage ./electronics { };
    nova-science = callPackage ./science { };
    nova-teleop-drive-joy = callPackage ./teleop_drive_joy { };
    nova-teleop-arm-joy = callPackage ./teleop_arm_joy { };
    nova-drive = callPackage ./drive/drive { };
    nova-drive-interfaces = callPackage ./drive/drive_interfaces { };
    nova-input-interfaces = callPackage ./inputs/input_interfaces { };
    nova-inputs = callPackage ./inputs/inputs { };
    nova-gimbal-cam = callPackage ./gimbal_cam { };
    nova-bringup = callPackage ./nova_bringup { };
    nova-interfaces = callPackage ./nova_interfaces { };
    nova-rover-description = callPackage ./rover_description { };
    nova-gazebo = callPackage ./nova_gazebo { };
    nova-python-control = callPackage ./python_control { };
    nova-python-control-old = callPackage ./python_control_old { };
    nova-excavation-construction = callPackage ./excavation_construction { };
    nova-utils = callPackage ./nova_utils { };
    lattice-primitive-generator = callPackage ./lattice_primitive_generator { };
  } // import ./arm { inherit pkgs; }
    // import ./auto { inherit pkgs; }
    // import ./blcmds { inherit pkgs; }
    // import ./cmds { inherit pkgs; }
    // import ./controllers { inherit pkgs; };

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
