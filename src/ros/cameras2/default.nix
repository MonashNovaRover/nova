{
  rosPackages = pkgs: with pkgs; {
    nova-cameras2 = callPackage ./nix/packages/cameras2 { };
  };
}
