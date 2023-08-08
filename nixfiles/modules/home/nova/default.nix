{ config, lib, ... }:

{
  config = lib.mkIf (config.home.username == "nova") {
    xdg.userDirs = {
      enable = true;
      createDirectories = true;
    };

    programs = {
      bash.enable = true;

      git.enable = true;

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
      };
    };
  };
}
