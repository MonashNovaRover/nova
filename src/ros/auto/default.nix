{ pkgs }:

with pkgs;

{
  nova-auto-bringup = callPackage ./auto_bringup { };
  nova-auto-start = callPackage ./auto_start { };
  lattice-primitive-generator = callPackage ./lattice_primitive_generator { };
  nova-behavior-tree = callPackage ./nova_behavior_tree { };
  nova-bt-navigators = callPackage ./nova_bt_navigators { };
  nova-costmap-2d = callPackage ./nova_costmap_2d { };
  nova-utils = callPackage ./nova_utils { };
} // import ./auto_object_localisation { inherit pkgs; }
