{ config, lib, ... }:

{
  config = lib.mkIf (config.home.username == "nova") {
    programs.bash.enable = true;
    xdg.userDirs = {
      enable = true;
      createDirectories = true;
    };
  };
}
