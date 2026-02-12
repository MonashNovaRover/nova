{ pkgs }:

with pkgs;

{
  nova-science = callPackage ./science { };
  nova-science-bringup = callPackage ./science_bringup { };
  nova-legacy-input-mode = callPackage ./science_control_modes/legacy_input_mode { };
  nova-science-interfaces = callPackage ./science_interfaces { };
  nova-teleop-science = callPackage ./teleop_science { };
}
