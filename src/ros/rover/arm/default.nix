{ pkgs }:

with pkgs;

{
  nova-arm-bringup = callPackage ./arm_bringup { };
  nova-arm-interfaces = callPackage ./arm_interfaces { };
  nova-auto-typing = callPackage ./auto_typing { };
  nova-gimbal-cam = callPackage ./old_arm/gimbal_cam { };
  nova-teleop-arm-joy = callPackage ./teleop_arm_joy { };
  nova-teleop-arm = callPackage ./teleop_arm { };
  teleop-turtle = callPackage ./teleop_turtle { };
}
// pkgs.lib.optionalAttrs (builtins.pathExists ../../../other/libcanmd/default.nix) {
  nova-arm = callPackage ./old_arm/arm { };
}
// import ./arm_kinematics { inherit pkgs; }
// import ./arm_controllers { inherit pkgs; }
// import ./arm_controllers_old { inherit pkgs; }
// import ./arm_control_modes { inherit pkgs; }
