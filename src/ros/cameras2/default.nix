{
  rosPackages = pkgs: with pkgs; {
    nova-camera-msgs = callPackage ./nix/packages/camera-msgs { };
    nova-cameras2 = callPackage ./nix/packages/cameras2 { };
  };
}
