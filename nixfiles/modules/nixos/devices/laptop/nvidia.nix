{ config, lib, ... }:

let
  cfg = config.devices.laptop.nvidia;
  nvidiaPackage = config.hardware.nvidia.package;
in
{
  options.devices.laptop.nvidia.enable = lib.mkEnableOption "Nova Rover base station laptop Nvidia configuration";

  config = lib.mkIf cfg.enable {

    # Enable nvidia caches
    nova.substituters.nvidia.enable = lib.mkDefault true;

    # Cuda support
    nixpkgs.config = {
      cudaSupport = lib.mkDefault false; # Only enable if tested properly
      cudaForwardCompat = lib.mkDefault true;
    };

    # Nvidia driver support
    hardware.nvidia = {
      modesetting.enable = lib.mkDefault true;
      dynamicBoost.enable = lib.mkDefault false; # Only for RTX 3000 and newer
      powerManagement.enable = lib.mkDeafult false; # Only enable if tested properly
      powerManagement.finegrained = lib.mkDefault false; # Only for RTX 2000 and newer
      open = lib.mkOverride 990 (nvidiaPackage ? open && nvidiaPackage ? firmware);
      package = lib.mkDefault config.boot.kernelPackages.nvidiaPackages.production;
      prime.offload = {
        # The 1050 Ti does not seem to like PRIME offloading. Check if newer ones like it
        enable = lib.mkDefault false;
        enableOffloadCmd = lib.mkDefault false;
      };
    };

    # Switching between igpu and nvidia gpu
    services.switcherooControl.enable = lib.mkDefault true;
    services.xserver.videoDrivers = lib.mkDefault [ "nvidia" ];
  };
}
