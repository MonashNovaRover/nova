{ supportedSystems
, nixpkgs
, home-manager
, nova-monorepo
, ...
}:

import ../../docs/mkdocs/default.nix {
  inherit
    supportedSystems
    nixpkgs
    home-manager
    nova-monorepo;
}
