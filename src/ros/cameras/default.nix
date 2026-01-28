{
  rosPackages = pkgs: with pkgs; {
    nova-oakenc = callPackage ../oak/nix { };
  };
}
