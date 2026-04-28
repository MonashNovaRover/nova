{
  rosPackages =
    pkgs: with pkgs; {
      nova-can-database = callPackage ./nix { };
    };
}
