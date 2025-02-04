{
  packages = pkgs: with pkgs; {
  };

  rosPackages = pkgs: with pkgs; {
    nova-gui = callPackage ./nix/packages/gui { };
    nova-gui-server = callPackage ./nix/packages/gui-server { };
  };
}