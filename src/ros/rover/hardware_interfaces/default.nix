{ pkgs }:

with pkgs;

{
  nova-blcmd-hardware = callPackage ./blcmd_hardware { };
  nova-blcmd-hardware2 = callPackage ./blcmd_hardware2 { };
  nova-cmd-hardware = callPackage ./cmd_hardware { };
  nova-qcmd-hardware = callPackage ./qcmd_hardware { };
  nova-blcmd-interfaces = callPackage ./old/blcmds/blcmd_interfaces { };
  nova-blcmd-utils = callPackage ./old/blcmds/blcmd_utils { };
  nova-cmd-interfaces = callPackage ./old/cmds/cmd_interfaces { };
  nova-cmd-utils = callPackage ./old/cmds/cmd_utils { };
}
