{ pkgs }:

with pkgs; {
    nova-blcmd-hardware = callPackage ./blcmd_hardware { };
    nova-blcmd-interfaces = callPackage ./blcmd_interfaces { };
    nova-blcmd-utils = callPackage ./blcmd_utils { };
}