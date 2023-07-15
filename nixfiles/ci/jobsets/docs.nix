{ supportedSystems
, nixpkgs
, home-manager
, src
}:

let
  lib = import ../lib.nix {
    inherit
      supportedSystems
      nixpkgs
      src;
    repos = [ ];
  };

  # We are building documentation, not native code.
  # The architecture doesn't matter. Use native system packages for speed.
  pkgs = import <nixpkgs> { };
  nova = lib.novaFor builtins.currentSystem;

  mkOptions = module: builtins.removeAttrs (pkgs.lib.evalModules {
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
            if (if builtins.isPath src then pkgs.lib.path.hasPrefix src else pkgs.lib.hasPrefix (toString src)) (/. + declaration) then
            rec {
              # Structure format described in nixos-render-docs:
              # https://github.com/NixOS/nixpkgs/blob/45ae0efbbce2aada6d5e8de6ace0c803b08ac9c7/pkgs/tools/nix/nixos-render-docs/src/nixos_render_docs/options.py#L52-L53
              name = (pkgs.lib.removePrefix ((toString src) + "/") declaration) +
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
    , options
    }: pkgs.stdenvNoCC.mkDerivation {
      name = pname;
      nativeBuildInputs = with pkgs; [
        python3Packages.mkdocs
        python3Packages.mkdocs-material
        nova.pkgs.python3Packages.mkdocs-safe-text-plugin
        nodejs
        jq
      ];
      dontUnpack = true;
      buildPhase = ''
        runHook preBuild

        mkdir -p docs
        ln -s ${pkgs.substitute {
          src = mkOptionsMd options;
          # For some reason, nixosOptionsDoc escapes periods. This is not
          # neccesary in CommonMark, and interferes with mkdocs ToC generation.
          # The <name> placeholder, used to describe submodule attribute sets,
          # also needs escaping.
          replacements = [
            "--replace" ''\.'' ''.''
            "--replace" ''\<name>'' ''&lt;name&gt;''
          ];
        }} docs/index.md
        
        mkdir -p docs/assets
        ln -s ${nova.pkgs.nova-icons}/share/icons/hicolor/16x16/apps/nova-logo-white-and-orange.png docs/assets/favicon.png
        ln -s ${nova.pkgs.nova-icons}/share/icons/hicolor/1024x1024/apps/nova-logo-white.png docs/assets/logo.png

        jq < '${pkgs.writeText "mkdocs.yaml" (builtins.toJSON {
          site_name = "${topic} Manual";

          nav = [
            { "Options" = "index.md"; }
          ];

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

  allHMOptions = mkOptions nova.homeModule;
  novaHMOptions = allHMOptions;

  allOSOptions = mkOptions nova.nixosModule;
  novaOSOptions = allOSOptions //
    { home-manager = { inherit (allOSOptions.home-manager) nova; }; };
in
{
  home-manager-docs = mkDocumentationSite {
    pname = "nova-home-manager-docs";
    topic = "Nova Rover Home Manager";
    options = novaHMOptions;
  };
  nixos-docs = mkDocumentationSite {
    pname = "nova-nixos-docs";
    topic = "Nova Rover NixOS";
    options = novaOSOptions;
  };
}
