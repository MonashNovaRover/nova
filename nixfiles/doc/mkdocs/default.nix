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
  nixfiles = nova-monorepo + "/nixfiles";
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
  helpers = import ./helpers.nix {
    inherit pkgs nixfiles;
  };
  mkOptions = helpers.mkOptions;
  mkOptionsMd = helpers.mkOptionsMd;

  mkDocumentationSite =
    { pname
    , topic
    , sections
    }:
    let
      sectionOutputs = builtins.listToAttrs (map
        (section: {
          name = "${section.path}.md";
          value = mkOptionsMd section.options;
        })
        sections);
    in pkgs.stdenvNoCC.mkDerivation {
      name = pname;
      src = ./. ;
      nativeBuildInputs = with pkgs; [
        python3Packages.mkdocs
        python3Packages.mkdocs-material
      ];
      dontUnpack = false;
      buildPhase = ''
        runHook preBuild

        ${pkgs.lib.concatMapStringsSep "\n" (section: ''
        ln -s ${sectionOutputs."${section.path}.md"} ${section.path}.md
        '') sections}

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
    };

  allHMOptions = mkOptions { module = (nixfiles + "/modules/home"); };
  novaHMOptions = allHMOptions;

  allOSOptions = mkOptions {
    module = (nixfiles + "/modules/nixos");
    specialArgs = { jetpack-nixos = null; };
  };
  novaOSOptions = allOSOptions //
    { home-manager = { inherit (allOSOptions.home-manager) nova; }; };

in
{
  docs = mkDocumentationSite {
    pname = "nova-mkdocs";
    topic = "Nova Rover";
    sections = [
      {
        title = "Home Manager";
        path = "home-manager";
        options = novaHMOptions;
      }
      {
        title = "NixOS";
        path = "nixos";
        options = novaOSOptions;
      }
    ];
  };
}