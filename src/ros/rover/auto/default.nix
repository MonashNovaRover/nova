{ pkgs }:

with pkgs;

{
  nova-auto-bringup = callPackage ./auto_bringup { };
  lattice-primitive-generator = callPackage ./lattice_primitive_generator { };
  nova-auto-interfaces = callPackage ./nova_auto_interfaces { };
  nova-behavior-tree = callPackage ./nova_behavior_tree { };
  nova-bt-navigators = callPackage ./nova_bt_navigators { };
  nova-costmap-2d = callPackage ./nova_costmap_2d { };
  nova-utils = callPackage ./nova_utils { };
} // import ./auto_object_localisation { inherit pkgs; }
