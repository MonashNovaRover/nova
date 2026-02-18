{ config, lib, ... }:

let
  cfg = config.devices.laptop.nvidia;
  nvidiaPackage = config.hardware.nvidia.package;
in
{
  options.devices.laptop.nvidia.enable = lib.mkEnableOption "Nova Rover base station laptop Nvidia configuration";

  config = lib.mkIf cfg.enable { 

    # Cuda support
    nixpkgs.config = {
      cudaSupport = false; # Only enable if tested properly
      cudaForwardCompat = true;
    };

    # Nvidia driver support
    hardware.nvidia = {
      modesetting.enable = true;
      powerManagement.enable = false; # Only enable if tested properly
      powerManagement.finegrained = false; # Only for RTX 2000 and newer
      open = lib.mkOverride 990 (nvidiaPackage ? open && nvidiaPackage ? firmware);
      package = config.boot.kernelPackages.nvidiaPackages.production;
      prime.offload = {
        # The 1050 Ti does not seem to like PRIME offloading. Check if newer ones like it
        enable = false;
        enableOffloadCmd = false;
      };
    };

    # Switching between igpu and nvidia gpu
    services.switcherooControl.enable = true;
    services.xserver.videoDrivers = lib.mkDefault [ "nvidia" ];

    # Enable nvidia caches
    nix.settings = {
      substituters = [
        "https://cache.nixos-cuda.org"
        "https://cache.flox.dev"
      ];
      trusted-public-keys = [
        "cuda-maintainers.cachix.org-1:0dq3bujKpuEPMCX6U4WylrUDZ9JyUG0VpVZa7CNfq5E="
        "flox-cache-public-1:7F4OyH7ZCnFhcze3fJdfyXYLQw/aV7GEed86nQ7IsOs="
      ];
    };
  };
}
