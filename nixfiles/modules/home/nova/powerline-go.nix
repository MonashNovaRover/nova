{ config, lib, ... }:

let
  cfg = config.nova.user;
in
{
  config = lib.mkIf cfg.enable {
    programs.powerline-go = {
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
}
