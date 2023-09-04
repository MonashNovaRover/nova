{ supportedSystems
, nixpkgs
, nixfiles
, slides
}:

import slides { pkgs = import nixpkgs { }; }
