{ pkgs }:

with pkgs;

{
  nova-blcmd-hardware = callPackage ./blcmd_hardware { };
  nova-cmd-hardware = callPackage ./cmd_hardware { };
  nova-blcmd-interfaces = callPackage ./old/blcmds/blcmd_interfaces { };
  nova-blcmd-utils = callPackage ./old/blcmds/blcmd_utils { };
  nova-cmd-interfaces = callPackage ./old/cmds/cmd_interfaces { };
  nova-cmd-utils = callPackage ./old/cmds/cmd_utils { };
}
