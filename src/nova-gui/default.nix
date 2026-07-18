{
  rosPackages = pkgs: with pkgs; {
    nova-gui = callPackage ./nix/packages/gui { };
    nova-gui-dev-shell = callPackage ./nix/packages/gui/dev-shell.nix { };
  };
}