{ config, lib, pkgs, ... }:

let
  cfg = config.nova.laptops.performance;
in
{
  options.nova.laptops.performance.enable = lib.mkEnableOption "Nova Rover base station laptop performance configuration";

  config = lib.mkIf cfg.enable {

    # Faster scheduler for flash store
    hardware.block = {
      scheduler = {
        "mmblk[0-9]*" = "none";
        "nvme[0-9]*" = "none";
        "sd[a-z]*" = "none";
      };
      defaultScheduler = "none";
    };

    # SSD Maintainence
    services.fstrim.enable = true;

    # Faster CPU scheduler
    services.scx = {
      enable = true;
      scheduler = "scx_lavd";
      extraArgs = [
        "--performance"
      ];
      package = pkgs.scx.rustscheds;
    };

    # Deprioritzes the nix daemon, increasing responsiveness for laptop
    nix = {
      daemonCPUSchedPolicy = "idle";
      daemonIOSchedClass = "idle";
      daemonIOSchedPriority = 7;
      gc = { # No gc for now
        #automatic = true;
        #dates = "monthly";
        #options = "--delete-older-than 30d";
      };
      # Reduces space after each build
      optimise = {
        automatic = true;
        dates = "weekly";
      };
    };
  };
}
