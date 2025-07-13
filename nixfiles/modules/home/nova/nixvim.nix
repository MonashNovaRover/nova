{ config, lib, ... }:

let
  nixvim = import (builtins.fetchGit {
    url = "https://github.com/nix-community/nixvim";
  });
  cfg = config.nova.desktop;
in
{
  imports = [
    nixvim.homeModules.nixvim
  ];
  config = lib.mkIf cfg.enable {
    programs.nixvim = {
      enable = true;
      viAlias = true;
      vimAlias = true;
      luaLoader.enable = true;
      performance = {
        byteCompileLua.enable = true;
        byteCompileLua.configs = true;
        byteCompileLua.initLua = true;
        byteCompileLua.luaLib = true;
        byteCompileLua.nvimRuntime = true;
        byteCompileLua.plugins = true;
        combinePlugins.enable = true;
      };
      colorschemes.catppuccin = {
        enable = true;
        settings = {
          color_overrides = {
            mocha = {
                    base = "#000000";
            };
                };
          flavour = "mocha";
        };
      };
      opts = {
        number = true;
        ignorecase = true;
        smartcase = true;
        hlsearch = true;
        foldlevel = 99;
        foldmethod = "indent";
        hidden = true;
        encoding = "utf-8";
        termguicolors = true;
        laststatus = 2;
        breakindent = true;
        showbreak = "↳";
        cursorlineopt = "number";
        clipboard = "unnamedplus";
        mouse = "a";
        shiftwidth = 2;
        tabstop = 2;
        expandtab = true;
      };

      plugins = {
        blink-cmp = {
          enable = true;
          settings = {
            fuzzy = {
              implementation = "rust";
            };
            completion = {
              ghost_text.enabled = true;
              documentation = {
                auto_show = true;
                auto_show_delay_ms = 500;
              };
            };
            snippets.preset = "luasnip";
          };
        };
        colorizer.enable = true;
        friendly-snippets.enable = true;
        lsp = {
          enable = true;
          servers = {

            pyright.enable = true;

            cmake.enable = true;
            clangd.enable = true;

            eslint.enable = true;
            cssls.enable = true;
            html.enable = true;

            jsonls.enable = true;

            nixd.enable = true;

            lua_ls.enable = true;

            tinymist.enable = true;

            zls.enable = true;

          };
        };
        luasnip = {
          enable = true;
          fromVscode = [{}]; # Enable friendly snippets
        };
        lz-n.enable = true;
        startify = {
          enable = true;
          settings = {
            enable_special = true;
            enable_unsafe = true;
            custom_header = [
              ""
  " @@   @@   @@@@@@  @@    @@   @@   @@@@@ @     @                 @@@@@@@@@@@@@@@@@@@@@@@  "
  " @@@ @@@  @@    @@ @@@@  @@  @@@@  @@    @     @            @@@@@@@@@@@@@@@@              "
  " @ @ @ @  @@    @@ @@@@@@@@ @@  @@ @@@@@ @@@@@@@         @@@@@@@@@@@@                     "
  "@@ @@@ @@ @@    @@ @@  @@@@ @@@@@@    @@ @     @      @@@@@@@@@@@                         "
  "@@  @  @@  @@@@@@  @@    @@ @    @ @@@@@ @     @    @@@@@@@@@@                            "
  "                                /                  @@@@@@@@@@                              "
  "                                                 @@@@@@@@@                                "
  "                  @@@@@    @@@@         @@@@@@@@@@ @@@@@@ @@@         @@@       @@@@      "
  "                  @@@@@@   @@@@       @@@@@@@@@@@@@@@@@    @@@       @@@       @@@@@@     "
  "                  @@@@@@@  @@@@      @@@@        @@@@      @@@       @@@      @@@  @@@    "
  "                  @@@@@@@@ @@@@      @@@          @@@       @@@     @@@      @@@    @@@   "
  "                  @@@@ @@@@@@@@      @@@          @@@        @@@   @@@      @@@      @@@  "
  "                  @@@@  @@@@@@@      @@@@        @@@@        @@@@ @@@@     @@@@@@@@@@@@@@ "
  "                  @@@@  @@@@@@@    @@@@@@@      @@@@          @@@@@@@     @@@@@@@@@@@@@@@@"
  "                  @@@@   @@@@@@   @@@@@ @@@@@@@@@@@            @@@@@      @@@@         @@@"
  "                                 @@@@@@@@  @@@@                                           "
  "                               @@@@@@@@@@                                                 "
  "                            @@@@@@@@@@@            @@@@@    @@@@@  @@    @@ @@@@@@ @@@@@  "
  "                         @@@@@@@@@@@@              @@  @@  @@   @@  @@  @@@ @@     @@  @@ "
  "                     @@@@@@@@@@@@@@                @@@@@  @@     @@  @@ @@  @@@@@@ @@@@@  "
  "               @@@@@@@@@@@@@@@@                    @@  @@  @@    @@   @@@   @@     @@  @@ "
  "  @@@@@@@@@@@@@@@@@@@@@@@@                         @@   @@  @@@@@@    @@@   @@@@@@ @@   @@"
  "                                                                                          "
  "    I use NixOs btw                                                                       "
            ];
            lists = [
              {
                type = "files";
                header = ["    Most Recently Used"];
              }
            ];
          };
        };
        snacks.enable = true;
        telescope = {
          enable = true;
          extensions.fzf-native.enable = true;
        };
        web-devicons.enable = true;
        yazi = {
          enable = true;
          settings.enable_mouse_support = true;
        };
      };
    };
  };
}
