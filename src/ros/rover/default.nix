{
  rosPackages = pkgs: with pkgs; {
    nova-bringup = callPackage ./nova_bringup { };
    nova-interfaces = callPackage ./nova_interfaces { };
    nova-rover-description = callPackage ./rover_description { };
    nova-excavation-construction = callPackage ./excavation_construction { };
    nova-controller-common = callPackage ./nova_controller_common { };

    ublox-dgnss-custom = callPackage ./nix/packages/ublox-dgnss { };

    # diff drive, pivot drive, strafe, 
  } // import ./arm { inherit pkgs; }
    // import ./auto { inherit pkgs; }
    // import ./chassis { inherit pkgs; }
    // import ./drive { inherit pkgs; }
    // import ./hardware_interfaces { inherit pkgs; }
    // import ./nova_generic { inherit pkgs; }
    // import ./old_inputs { inherit pkgs; }
    // import ./science { inherit pkgs; }
    // import ./simulations { inherit pkgs; };

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
