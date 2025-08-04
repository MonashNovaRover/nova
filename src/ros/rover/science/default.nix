{ pkgs }:

with pkgs;

{
  nova-science = callPackage ./science { };
  nova-legacy-input-mode = callPackage ./science_control_modes/legacy_input_mode { };
  nova-teleop-science = callPackage ./teleop_science { };
}
