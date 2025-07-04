{ pkgs }:

with pkgs; {
    nova-arm = callPackage ./arm { };
    nova-arm-bringup = callPackage ./arm_bringup { };
    nova-arm-interfaces = callPackage ./arm_interfaces { };
    nova-auto-typing = callPackage ./auto_typing { };
}