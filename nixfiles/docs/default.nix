{ supportedSystems
, nixpkgs
, home-manager
, nova-monorepo
, ...
}:

let
  nixfiles = nova-monorepo + "/nixfiles";
  lib = import ../ci/lib.nix {
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

  mkOptions = { module, specialArgs ? { } }: builtins.removeAttrs (pkgs.lib.evalModules {
    inherit specialArgs;
    modules = [
      { _module.check = false; }
      module
    ];
  }).options [ "_module" ];

  mkOptionsMd = options: (pkgs.nixosOptionsDoc {
    inherit options;
    documentType = "none";
    transformOptions = option:
      # Inspired by Nixpkgs:
      # https://github.com/NixOS/nixpkgs/blob/45ae0efbbce2aada6d5e8de6ace0c803b08ac9c7/doc/default.nix#L75-L86
      option // {
        declarations = map
          (declaration:
            let
              isLocalModule =
                if builtins.isPath nixfiles then
                  pkgs.lib.path.hasPrefix nixfiles (/. + declaration)
                else
                  pkgs.lib.hasPrefix (toString nixfiles) (toString declaration);
            in
            if isLocalModule then
              rec {
                # Structure format described in nixos-render-docs:
                # https://github.com/NixOS/nixpkgs/blob/45ae0efbbce2aada6d5e8de6ace0c803b08ac9c7/pkgs/tools/nix/nixos-render-docs/src/nixos_render_docs/options.py#L52-L53
                name = (pkgs.lib.removePrefix ((toString nixfiles) + "/") declaration) +
                (pkgs.lib.optionalString (builtins.readFileType declaration == "directory") "/default.nix");
                url = "https://github.com/MonashNovaRover/nixfiles/blob/master/${name}";
                # TODO: Is there a way to generate URLs with a fixed Git revision?
              }
            else declaration
          )
          option.declarations;
      };
  }).optionsCommonMark;

  mkDocumentationSite =
    { pname
    , topic
    , overviewMarkdown
    , sections
    }:
    let
      nav = [
        { "Overview" = "index.md"; }
      ] ++ map (section: { "${section.title}" = "${section.path}.md"; }) sections;

      sectionOutputs = builtins.listToAttrs (map
        (section: {
          name = "${section.path}.md";
          value = mkOptionsMd section.options;
        })
        sections);
    in pkgs.stdenvNoCC.mkDerivation {
      name = pname;
      nativeBuildInputs = with pkgs; [
        python3Packages.mkdocs
        python3Packages.mkdocs-material
        jq
      ];
      dontUnpack = true;
      buildPhase = ''
        runHook preBuild

        mkdir -p docs
        cat > docs/index.md <<'EOF'
${overviewMarkdown}
EOF

        ${pkgs.lib.concatMapStringsSep "\n" (section: ''
        ln -s ${sectionOutputs."${section.path}.md"} docs/${section.path}.md
        '') sections}

        mkdir -p docs/assets
        ln -s ${nova.pkgs.nova-icons}/share/icons/hicolor/16x16/apps/nova-logo-white-and-orange.png docs/assets/favicon.png
        ln -s ${nova.pkgs.nova-icons}/share/icons/hicolor/1024x1024/apps/nova-logo-white.png docs/assets/logo.png

        jq < '${pkgs.writeText "mkdocs.yaml" (builtins.toJSON {
          site_name = "${topic} Manual";

          inherit nav;

          theme = {
            name = "material";
            favicon = "assets/favicon.png";
            logo = "assets/logo.png";
            palette = [
              # TODO: Use colours from branding standards once a full palette is established.
              {
                media = "(prefers-color-scheme: light)";
                scheme = "default";
                primary = "pink";
                toggle = {
                  icon = "material/brightness-7";
                  name = "Switch to dark mode";
                };
              }
              {
                media = "(prefers-color-scheme: dark)";
                scheme = "slate";
                primary = "black";
                toggle = {
                  icon = "material/brightness-4";
                  name = "Switch to light mode";
                };
              }
            ];
            features = [
              "navigation.tracking"
              "navigation.sections"
              "navigation.expand"
              "search.suggest"
              "search.share"
            ];
          };

          markdown_extensions.toc.permalink = true;

          plugins.search = {
            separator = ''[\s\-\.]+'';
          };
        })}' ".site_dir = \"$out/share/doc/${pname}\"" > mkdocs.yml

        mkdocs build

        runHook postBuild
      '';

      postInstall = ''
        mkdir -p "$out/nix-support"
        echo "doc manual $out/share/doc/${pname}" > "$out/nix-support/hydra-build-products"
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

  docsOverview = ''
# Overview

This manual is  and Home Manager module trees.

Use the subpages to browse the option sets that are currently documented:

- Home Manager - generated from `nixfiles/modules/home`
- NixOS - generated from `nixfiles/modules/nixos`

To add another documentation section later, extend the `sections` list in this
file with another entry.

Jetson support depends on the external `jetpack-nixos` source. To keep this
documentation build offline and restricted-mode safe, that external module is
not imported here, so Jetson-specific upstream options are omitted from the
generated option list.
'';
in
{
  docs = mkDocumentationSite {
    pname = "nova-docs";
    topic = "Nova Rover";
    overviewMarkdown = docsOverview;
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