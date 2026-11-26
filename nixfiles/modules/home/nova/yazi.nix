{ lib, pkgs, config, ... }:

let
  cfg = config.nova.user;
in
{
  config = lib.mkIf cfg.enable {
    programs.yazi = {
      enable = true;
      shellWrapperName = "y";

      # Declare and install the Rosé Pine flavor
      flavors = {
        dracula = pkgs.fetchFromGitHub {
          owner = "yazi-rs";
          repo = "flavors";
          rev = "main";
          sha256 = "sha256-G0N+dgkmuadPBYHr0a912CFGmisv+lQ+wAlkPG1kU68=";
          postFetch = "mv $out/dracula.yazi/* $out && rm -rf $out/catppuccin-frappe.yazi $out $out/catppuccin-latte.yazi $out/catppuccin-macchiato.yazi $out/catppuccin-mocha.yazi $out/dracula-frappe.yazi $out/scripts";
        };

        sequoia = pkgs.fetchFromGitHub {
          owner = "cisco336";
          repo = "sequoia.yazi";
          rev = "main";
          sha256 = "sha256-G0N+dgkmuadPBYHr0a912CFGmisv+lQ+wAlkPG1kU68=";
        };
      };

      plugins = {
        inherit (pkgs.yaziPlugins) githead ouch;
      };

      theme = {
        flavor = {
          light = "sequoia";
          dark = "dracula";
        };

        mgr = {
          cwd = {
            fg = "#f5c2e7";
            bold = true;
          };
        };

        # Get rid of background
        manager = {
          parent_style  = { bg = "reset"; };
          current_style = { bg = "reset"; };
          preview_style = { bg = "reset"; };
        };
      };

      settings = {
        yazi = {
          mgr = {
            sort_by = "alphabetical";
            sort_sensitive = true;
            sort_dir_first = true;
            linemode = "size";
            show_symlink = true;
          };
          preview = {
            wrap = "yes";
            tab_size = 8;
            image_delay = 16;
            image_filter = "catmull-rom";
            image_quality = 50;
            sixel_fraction = 20;
          };
          plugin = {
            prepend_previewers = [
              { mime = "application/{*zip,tar,bzip2,7z*,rar,xz,zstd,java-archive}"; run = "ouch"; }
            ];
          };
          opener.open = [
            { run = "xdg-open \"$@\""; desc = "Open"; }
          ];
        };
        keymap.manager.prepend_keymap = [
          {
            on = [ "d" ];
            run = "remove --permanently";
          }
        ];
      };
      initLua = ''
        require("githead"):setup({
          order = {
            "__spacer__",
            "branch",
            "remote_branch",
            "__spacer__",
            "tag",
            "__spacer__",
            "commit",
            "__spacer__",
            "behind_ahead_remote",
            "__spacer__",
            "stashes",
            "__spacer__",
            "state",
            "__spacer__",
            "staged",
            "__spacer__",
            "unstaged",
            "__spacer__",
            "untracked",
          },

          branch_prefix = "",
          branch_color = "#89b4fa",

          remote_branch_prefix = "󰊢",
          remote_branch_color = "#cba6f7",

          tag_prefix = "󰓹",

          always_show_commit = true,
          commit_prefix = "󰜏",
          commit_color = "#f9e2af",

          stashes_prefix = "󰏗",
          stashed_color = "#f9e2af",

          show_behind_ahead_remote = true,
          behind_remote_symbol = "⇡ ",
          ahead_remote_symbol = "⇡ ",
          behind_remote_color = "#f38ba8",
          ahead_remote_color = "#89b4fa",

          show_state = true,
          show_state_prefix = false,
          state_symbol = "!! ",
          state_color = "#B58901",

          staged_symbol = "● ",
          staged_color = "#a6e3a1",

          unstaged_symbol = "✎ ",
          unstaged_color = "#fab387",

          untracked_symbol = "? ",
          untracked_color = "#f38ba8", 
        })
      '';
    };
  };
}
