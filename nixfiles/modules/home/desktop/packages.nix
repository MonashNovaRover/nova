{ config, pkgs, lib, ... }:

let
  cfg = config.nova.desktop;
in
{
  config = lib.mkIf cfg.enable {
    home.packages = with pkgs; [ 
      can-utils 
    ];
  };
}
