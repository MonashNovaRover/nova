{ pkgs }:

with pkgs; {
    nova-auto-interfaces = callPackage ./nova_auto_interfaces { };
    nova-behavior-tree = callPackage ./nova_behavior_tree { };
    nova-bt-navigators = callPackage ./nova_bt_navigators { };
    nova-costmap-2d = callPackage ./nova_costmap_2d { };
    nova-detection-overlay = callPackage ./nova_detection_overlay { };
    nova-object-localisation = callPackage ./nova_object_localisation { };
}