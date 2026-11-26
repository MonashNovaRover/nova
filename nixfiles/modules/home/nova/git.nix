{ config, lib, ... }:

let
  cfg = config.nova.user;
in
{
  config = lib.mkIf cfg.enable {
    programs = {
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
    };
  };
}
