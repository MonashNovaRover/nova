{ config, lib, ... }:

let
  cfg = config.nova.desktop.browser;
in
{
  options.nova.desktop.browser.enable = lib.mkEnableOption "Nova Rover user Chromium configuration";

  config = lib.mkIf cfg.enable {
    programs.chromium = {
      enable = true;
      commandLineArgs = [
        "--profile-directory=nova"
      ];
    };
  };
}
