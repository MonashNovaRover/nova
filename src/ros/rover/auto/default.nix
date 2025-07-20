{ pkgs }:

with pkgs;

{
  nova-auto-bringup = callPackage ./auto_bringup { };
  lattice-primitive-generator = callPackage ./lattice_primitive_generator { };
  nova-auto-interfaces = callPackage ./nav2_autonomous/nova_auto_interfaces { };
  nova-behavior-tree = callPackage ./nav2_autonomous/nova_behavior_tree { };
  nova-bt-navigators = callPackage ./nav2_autonomous/nova_bt_navigators { };
  nova-costmap-2d = callPackage ./nav2_autonomous/nova_costmap_2d { };
  nova-detection-overlay = callPackage ./nav2_autonomous/nova_detection_overlay { };
  nova-object-localisation = callPackage ./nav2_autonomous/nova_object_localisation { };
  nova-pointcloud-filter = callPackage ./nav2_autonomous/nova_pointcloud_filter { };
  nova-utils = callPackage ./nova_utils { };
}
