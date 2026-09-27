{ pkgs }:

with pkgs;

let
  libblcmdAvailable = builtins.pathExists ../../../other/libblcmd/default.nix;
in
{
  nova-drive-bringup = callPackage ./drive_bringup { };
  nova-teleop-drive-joy = callPackage ./teleop_drive_joy { };
  nova-drive-interfaces = callPackage ./drive_interfaces { };
}
// (if libblcmdAvailable then {
  nova-drive = callPackage ./old_drive/drive { };
} else {})
// import ./drive_controllers { inherit pkgs; }
