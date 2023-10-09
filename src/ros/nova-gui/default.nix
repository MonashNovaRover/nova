{
  rosPackages = pkgs: with pkgs; {
    nova-gui = callPackage ./nix/packages/gui { };
  };
}