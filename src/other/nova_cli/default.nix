{
  pythonPackages = pkgs: with pkgs; {
    nova-cli = callPackage ./package.nix { };
  };
}
