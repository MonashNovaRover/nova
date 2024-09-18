{
  rosPackages = pkgs: with pkgs; {
    nova-camera-msgs = callPackage ./nix/packages/camera-msgs { };
    nova-cameras2 = callPackage ./nix/packages/cameras2 { };
  };

  shellAliases = rec {
    cameras = "ros2 launch cameras2 camera_server_launch.py platform:=rover";
    cameras_all = "${cameras} autostart:=true";
  };
}
