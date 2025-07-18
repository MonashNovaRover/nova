{ pkgs }:

with pkgs;

{
  nova-auto-bringup = callPackage ./auto_bringup { };
  lattice-primitive-generator = callPackage ./lattice_primitive_generator { };
  nova-auto-interfaces = callPackage ./nav2_autonomous/nova-auto-interfaces { };
  nova-behavior-tree = callPackage ./nav2_autonomous/nova-behavior-tree { };
  nova-bt-navigators = callPackage ./nav2_autonomous/nova-bt-navigators { };
  nova-costmap-2d = callPackage ./nav2_autonomous/nova_costmap_2d { };
  nova-detection-overlay = callPackage ./nav2_autonomous/nova-detection-overlay { };
  nova-object-localisation = callPackage ./nav2_autonomous/nova-object-localisation { };
  nova-pointcloud-filter = callPackage ./nnav2_autonomous/nova-pointcloud-filter { };
  nova-utils = callPackage ./nova_utils { };
}
