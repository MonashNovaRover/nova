{
  rosPackages = pkgs: with pkgs; {
    nova-core = callPackage ./nix/packages/core { };
    nova-control = callPackage ./nix/packages/control { };
    nova-autonomous = callPackage ./nix/packages/autonomous { };
    nova-electronics = callPackage ./nix/packages/electronics { };
    nova-science = callPackage ./nix/packages/science { };
  };
}
