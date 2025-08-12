{ pkgs }:

with pkgs;

{
  nova-arm-bringup = callPackage ./arm_bringup { };
  nova-arm-interfaces = callPackage ./arm_interfaces { };
  nova-auto-typing = callPackage ./auto_typing { };
  nova-arm = callPackage ./old_arm/arm { };
  nova-gimbal-cam = callPackage ./old_arm/gimbal_cam { };
  nova-teleop-arm-joy = callPackage ./teleop_arm_joy { };
  nova-teleop-arm = callPackage ./teleop_arm { };
  teleop-turtle = callPackage ./teleop_turtle { };
}
// import ./arm_controllers { inherit pkgs; }
// import ./arm_control_modes { inherit pkgs; }
