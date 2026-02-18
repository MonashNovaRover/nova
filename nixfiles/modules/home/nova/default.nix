{ config, pkgs, lib, ... }:

let
  cfg = config.nova.user;
in
{
  options.nova.user.enable =
    lib.mkEnableOption "standard Nova Rover user settings" // {
      default = config.home.username == "nova";
      defaultText = lib.literalExpression ''"nova"'';
    };

  config = lib.mkIf cfg.enable {
    nova = {
      macros.enable = true;
    };

    xdg.userDirs = {
      enable = true;
      createDirectories = true;
    };

    programs = {
      bash.enable = true;
      command-not-found.enable = true;

      git = {
        enable = true;
        settings.user = {
          name = "Monash Nova Rover";
          email = "novaroverteam@monash.edu";
        };
        lfs = {
          enable = true;
        };
      };
      gh.enable = true;

      powerline-go = {
        enable = true;
        modules = [
          "nix-shell"
          "venv"
          "user"
          "host"
          "ssh"
          "cwd"
          "perms"
          "git"
          "jobs"
          "exit"
          "root"
        ];
      };

      vim = {
        enable = true;
        extraConfig = ''
          inoremap jk <Esc>
          set mouse=a
          set tabstop=2
          set shiftwidth=2
          set expandtab
          set clipboard=unnamedplus
        '';
        defaultEditor = true;
      };

      tmux = {
        enable = true;
      };
    };

    home = {
      sessionVariables = {
        NIX_AUTO_RUN = "1";
        NIX_AUTO_RUN_INTERACTIVE = "1";
      };

      shellAliases = rec {
        # Default options
        grep = "grep --color=auto";

        # Option shortcuts
        ll = "ls -l";
        la = "ls -a";
        lla = "ls -la";

        # Drop-in replacements
        sudo = "sudo ";

        # N00b precautions
        nix-env = "${pkgs.writeShellScript "nix-env-stub" ''
          echo >&2 'Careful there: nix-env manages packages imperatively, which should not be done.'
          echo >&2 'Are you sure you can'"'"'t use nix-shell -p (or simply run a command and choose a package when prompted)?'
          echo >&2 'If you are absolutely sure you want to use nix-env, it can be run with an escape like so: \nix-env'
          exit 1
        ''}";
        apt = "${pkgs.writeShellScript "apt-stub" ''
          echo >&2 'This is NixOS, not Ubuntu; apt is not installed. To use a new program, you can:'
          echo >&2 '  a) Run the command, and choose the desired package when prompted'
          echo >&2 '  b) Open a temporary shell with the desired package available using nix-shell -p <package>'
          echo >&2 '  c) Permanently install the package by adding it to the NixOS or Home Manager configuration'
          echo >&2 'Feel free to reach out to a software subteam member for further explanation.'
          exit 1
        ''}";
        microsoft-edge = "xdg-open 'https://www.youtube.com/watch?v=dQw4w9WgXcQ'";
        edge = microsoft-edge;
      };

      packages = with pkgs; builtins.filter (lib.meta.availableOn stdenv.hostPlatform) ([
        # Shell utilities
        pciutils
        usbutils
        gpsd
        v4l-utils
        btop
        nix-output-monitor
      ] ++ lib.optionals config.nova.desktop.enable [
        # Desktop apps
        gitkraken
      ]);
    };
  };
}
