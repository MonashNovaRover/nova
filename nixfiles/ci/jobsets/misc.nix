{ supportedSystems
, nixpkgs
, nova-monorepo
, rosDistro
, ...
}@args:

let
  nixfiles = nova-monorepo + "/nixfiles";
  lib = import ../lib.nix {
    inherit
      supportedSystems
      nixpkgs
      nova-monorepo;
    repoNames = [ ];
  };
in
lib.novaForAllSystems (nova: {
  inherit (if rosDistro == null then nova.pkgs.ros else nova.pkgs.rosPackages.${rosDistro})
    # Autonomous
    librealsense2-gui

    # Arm
    moveit-ros

    # Temporary
    ## Gazebo and ros2-control
    ros-gz
    gz-ros2-control
    ros2-control
    ros2-controllers
    pluginlib
    ## Nav2
    nav2-bringup
    navigation2
    ;
  # Non ROS things:
  inherit (nova.pkgs)
    # Novacarrier Flash
    # this may only work on x86
    novacarrier-flash
    ;
})
