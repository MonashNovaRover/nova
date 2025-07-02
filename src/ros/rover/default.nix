{
  rosPackages = pkgs: with pkgs; {
    nova-electronics = callPackage ./electronics { };
    nova-science = callPackage ./science { };
    nova-blcmd-hardware = callPackage ./hardware_interfaces/blcmd_hardware { };
    nova-cmd-hardware = callPackage ./hardware_interfaces/cmd_hardware { };
    nova-costmap-2d = callPackage ./nav2_autonomous/nova_costmap_2d { };
    nova-behavior-tree = callPackage ./nav2_autonomous/nova_behavior_tree { };
    nova-object-localisation = callPackage ./nav2_autonomous/nova_object_localisation { };
    nova-teleop-drive-joy = callPackage ./teleop_drive_joy { };
    nova-teleop-arm-joy = callPackage ./teleop_arm_joy { };
    nova-pointcloud-filter = callPackage ./nav2_autonomous/nova_pointcloud_filter { };
    nova-drive = callPackage ./drive/drive { };
    nova-drive-interfaces = callPackage ./drive/drive_interfaces { };
    nova-blcmd-interfaces = callPackage ./blcmds/blcmd_interfaces { };
    nova-blcmd-utils = callPackage ./blcmds/blcmd_utils { };
    nova-arm-interfaces = callPackage ./arm/arm_interfaces { };
    nova-arm = callPackage ./arm/arm { };
    nova-auto-typing = callPackage ./arm/auto_typing { };
    nova-input-interfaces = callPackage ./inputs/input_interfaces { };
    nova-inputs = callPackage ./inputs/inputs { };
    nova-cmd-interfaces = callPackage ./cmds/cmd_interfaces { };
    nova-cmd-utils = callPackage ./cmds/cmd_utils { };
    nova-gimbal-cam = callPackage ./gimbal_cam { };
    nova-bringup = callPackage ./nova_bringup { };
    nova-auto-bringup = callPackage ./auto_bringup { };
    nova-arm-bringup = callPackage ./arm_bringup { };
    nova-interfaces = callPackage ./nova_interfaces { };
    nova-rover-description = callPackage ./rover_description { };
    nova-gazebo = callPackage ./nova_gazebo { };
    nova-python-control = callPackage ./python_control { };
    nova-python-control-old = callPackage ./python_control_old { };
    nova-excavation-construction = callPackage ./excavation_construction { };
    nova-detection-overlay = callPackage ./nav2_autonomous/nova_detection_overlay { };
    nova-bt-navigators = callPackage ./nav2_autonomous/nova_bt_navigators { };
    nova-auto-interfaces = callPackage ./nav2_autonomous/nova_auto_interfaces { };
    nova-utils = callPackage ./nova_utils { };
    lattice-primitive-generator = callPackage ./lattice_primitive_generator { };
    nova-pivot-drive-controller = callPackage ./controllers/pivot_drive_controller { };
    nova-strafe-controller = callPackage ./controllers/strafe_controller { };
    nova-diff-drive-controller = callPackage ./controllers/nova_diff_drive_controller { };
    nova-arm-controller = callPackage ./controllers/nova_arm_controller { };
    nova-twistmapper = callPackage ./controllers/nova_twistmapper { };
    nova-path-planner = callPackage ./controllers/nova_path_planner { };
    nova-banksia-kinematics-plugin = callPackage ./controllers/banksia_kinematics_plugin { };
    nova-waratah-kinematics-plugin = callPackage ./controllers/waratah_kinematics_plugin { };
  };

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
