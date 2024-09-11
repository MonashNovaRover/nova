{ config, pkgs, lib, ... }:

let
  cfg = config.nova.workspace;
in
{
  imports = [
    ./services.nix
  ];

  options.nova.workspace = {
    enable = lib.mkEnableOption "the Nova Rover ROS workspace";
    package = lib.mkPackageOption pkgs [ "nova" "ros" "nova-workspace" ] { };
  };

  config.home-manager.nova.sharedModules = [{
    nova.workspace = {
      inherit (cfg) enable package;
      gui.enable = lib.mkDefault config.services.xserver.enable;
    };
  }];
}
