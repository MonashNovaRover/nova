{ config, pkgs, lib, ... }:

let
  cfg = config.nova.user;
in
{

  imports = [
    ./git.nix
    ./home.nix
    ./powerline-go.nix
    ./vim.nix
    ./yazi.nix
    ./zellij.nix
  ];

  options.nova.user.enable =
    lib.mkEnableOption "standard Nova Rover user settings" // {
      default = config.home.username == "nova";
      defaultText = lib.literalExpression ''"nova"'';
    };

  config = lib.mkIf cfg.enable {
    nova = {
      macros.enable = true;
      monitoring.enable = true;
    };

    xdg.userDirs = {
      enable = true;
      createDirectories = true;
    };

    programs = {
      bash.enable = true;
      command-not-found.enable = true;
    };
  };
}
