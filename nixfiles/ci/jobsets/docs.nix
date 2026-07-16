{ supportedSystems
, nixpkgs
, home-manager
, nova-monorepo
, ...
}:

import ../../docs/default.nix {
  inherit
    supportedSystems
    nixpkgs
    home-manager
    nova-monorepo;
}
