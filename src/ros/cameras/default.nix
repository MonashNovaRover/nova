{
  rosPackages = pkgs: with pkgs; {
    nova-camera-msgs = callPackage ./camera_msgs { };
    nova-cameras = callPackage ./cameras3 { };
  };
}
