{ pkgs }:

with pkgs;

{
  nova-teleop-drive-joy = callPackage ./teleop_drive_joy { };
  nova-drive = callPackage ./old_drive/drive { };
  nova-drive-interfaces = callPackage ./drive_interfaces { };
  nova-teleop-drive = callPackage ./teleop_drive { };
} // import ./drive_controllers { inherit pkgs; }
