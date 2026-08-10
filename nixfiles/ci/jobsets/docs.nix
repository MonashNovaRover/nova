{ supportedSystems
, nixpkgs
, home-manager
, nova-monorepo
, ...
}:
{
  docs = import ../../doc/mkdocs/default.nix {
    inherit
      supportedSystems
      nixpkgs
      home-manager
      nova-monorepo;
  };
}
