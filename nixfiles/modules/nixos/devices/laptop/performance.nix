{ config, lib, pkgs, ... }:

let
  cfg = config.devices.laptop.performance;
in
{
  options.devices.laptop.performance.enable = lib.mkEnableOption "Nova Rover base station laptop performance configuration";

  config = lib.mkIf cfg.enable {

    # Faster scheduler for flash store
    hardware.block = {
      scheduler = lib.mkDefault {
        "mmblk[0-9]*" = "none";
        "nvme[0-9]*" = "none";
        "sd[a-z]*" = "none";
      };
      defaultScheduler = lib.mkDefault "none";
    };

    # SSD Maintainence
    services.fstrim.enable = lib.mkDefault true;

    # Faster CPU scheduler
    services.scx = {
      enable = lib.mkDefault true;
      scheduler = lib.mkDefault "scx_lavd";
      extraArgs = lib.mkDefault [
        "--autopower"
      ];
      package = lib.mkDefault pkgs.scx.rustscheds;
    };

    # Deprioritzes the nix daemon, increasing responsiveness for laptop
    nix = {
      daemonCPUSchedPolicy = lib.mkDefault "idle";
      daemonIOSchedClass = lib.mkDefault "idle";
      daemonIOSchedPriority = lib.mkDefault 7;

      # Reduces space after each build
      optimise = {
        automatic = lib.mkDefault true;
        dates = lib.mkDefault "weekly";
      };
    };
  };
}
