{ pkgs }:

with pkgs;

{
  nova-detection-overlay = callPackage ./nova_detection_overlay { };
  nova-object-localisation = callPackage ./nova_object_localisation { };
}
