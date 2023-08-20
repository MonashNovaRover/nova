{ supportedSystems
, nixpkgs
, src
, rosDistro
, ...
}@args:

let
  lib = import ../lib.nix {
    inherit
      supportedSystems
      nixpkgs
      src;
    repoNames = [ ];
  };
in
lib.novaForAllSystems (nova: {
  inherit (if rosDistro == null then nova.pkgs.ros else nova.pkgs.rosPackages.${rosDistro})
    # Gazebo and ros2-control
    gazebo-ros-pkgs
    ros2-control
    ros2-controllers
    gazebo-ros2-control
    pluginlib;
})
