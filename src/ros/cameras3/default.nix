{
  rosPackages = pkgs: with pkgs; {
    #nova-camera-msgs = callPackage ./camera_msgs { };
    nova-cameras3 = callPackage ./cameras3 { };
  };

  # shellAliases = rec {
  #   cameras = "ros2 launch cameras2 camera_server_launch.py platform:=rover";
  #   cameras_all = "${cameras} autostart:=true";
  # };
}