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
  };
}
