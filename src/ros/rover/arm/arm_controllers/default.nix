{ pkgs }:

with pkgs;

{
  nova-arm-controller = callPackage ./nova_arm_controller { };
  nova-twistmapper = callPackage ./nova_twistmapper { };
  nova-path-planner = callPackage ./nova_path_planner { };
}
