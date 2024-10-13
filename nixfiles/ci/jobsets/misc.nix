{ supportedSystems
, nixpkgs
, nixfiles
, rosDistro
, ...
}@args:

let
  lib = import ../lib.nix {
    inherit
      supportedSystems
      nixpkgs
      nixfiles;
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
    gazebo-ros-pkgs
    ros2-control
    ros2-controllers
    gazebo-ros2-control
    pluginlib
    ## Nav2
    nav2-bringup
    navigation2;
})
