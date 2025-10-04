{
  pythonPackages = pkgs: with pkgs; {
    nova-can-sleuth = callPackage ./nix/packages/can-sleuth { };
  };
}

