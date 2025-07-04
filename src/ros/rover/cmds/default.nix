{ pkgs }:

with pkgs; {
    nova-cmd-hardware = callPackage ./cmd_hardware { };
    nova-cmd-interfaces = callPackage ./cmd_interfaces { };
    nova-cmd-utils = callPackage ./cmd_utils { };
}