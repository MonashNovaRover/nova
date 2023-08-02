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
    };

    home.sessionVariables = {
      NIX_AUTO_RUN = "1";
      NIX_AUTO_RUN_INTERACTIVE = "1";
    };
  };
}
