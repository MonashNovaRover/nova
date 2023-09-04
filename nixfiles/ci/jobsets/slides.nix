{ supportedSystems
, nixpkgs
, slides
}:

import slides { pkgs = import nixpkgs { }; }
