{
  rosPackages = pkgs: with pkgs; {
    nova-gui = callPackage ./nix/packages/gui { };
    nova-gui-test = callPackage ./nix/packages/gui/test.nix { };
  };
}