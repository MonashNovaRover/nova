{ supportedSystems
, nixpkgs
, home-manager
, nova-monorepo
, ...
}:

/*
  Nix file which generates documentation using MkDocs.
 */

let
  lib = import ../../ci/lib.nix {
    inherit
      supportedSystems
      nixpkgs
      nova-monorepo;
    repoNames = [ ];
  };

  # We are building documentation, not native code.
  # The architecture doesn't matter. Use native system packages for speed.
  pkgs = import nixpkgs { };
  nova = lib.novaFor builtins.currentSystem;
  handleNixOptions = import ./nix-option-schema-helper.nix {
    inherit pkgs nova-monorepo;
  };

  nix-options = [
    {
      path = "home-manager";
      page = handleNixOptions { module = nova-monorepo + "/nixfiles/modules/home"; };
    }
    {
      path = "nixos";
      page = handleNixOptions {
        module = nova-monorepo + "/nixfiles/modules/nixos";
        specialArgs = { jetpack-nixos = null; };
      };
    }
  ];

in

pkgs.stdenvNoCC.mkDerivation {
  name = "nova-mkdocs";
  src = ./. ;
  nativeBuildInputs = with pkgs; [
    python3Packages.mkdocs
    python3Packages.mkdocs-material
  ];
  dontUnpack = false;
  buildPhase = ''
    runHook preBuild

    ${pkgs.lib.concatMapStringsSep "\n" (section: ''
    ln -s ${section.page} ${section.path}.md
    '') nix-options}

    mkdir -p assets
    ln -s ${nova.pkgs.nova-icons}/share/icons/hicolor/16x16/apps/nova-logo-white-and-orange.png assets/favicon.png
    ln -s ${nova.pkgs.nova-icons}/share/icons/hicolor/1024x1024/apps/nova-logo-white.png assets/logo.png

    mv ../$(basename "$PWD") ../docs && cd ..
    mv docs/mkdocs.yml mkdocs.yml

    mkdocs build -d $out/docs

    runHook postBuild
  '';

  postInstall = ''
    mkdir -p "$out/nix-support"
    echo "doc nova-documentation $out/docs" > "$out/nix-support/hydra-build-products"
  '';
}