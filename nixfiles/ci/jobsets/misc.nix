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

    ament-cmake-core 
    ament-cmake-export-definitions 
    ament-cmake-export-dependencies 
    ament-cmake-export-include-directories 
    ament-cmake-export-interfaces 
    ament-cmake-export-libraries 
    ament-cmake-export-link-flags 
    ament-cmake-export-targets 
    ament-cmake-gen-version-h 
    ament-cmake-libraries 
    ament-cmake-python 
    ament-cmake-target-dependencies 
    ament-cmake-test 
    ament-cmake-version 
    cmake
    ;
})
