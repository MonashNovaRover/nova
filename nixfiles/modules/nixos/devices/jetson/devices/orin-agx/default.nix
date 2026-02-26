{ config, lib, ... }:

let
  cfg = config.devices.jetson.orin-agx;
in
{
  imports = [
    ./boot
  ];

  options.devices.jetson.orin-agx.enable = lib.mkEnableOption "configuration for the NVIDIA Jetson Orin AGX";

  config = lib.mkIf cfg.enable {
    devices.jetson.enable = true;
    hardware.nvidia-jetpack = {
      enable = true;
      som = "orin-agx";
      super = true; # Super speed
    };

    # Apply global cuda overlays
    nixpkgs.overlays = [
      (final: _: { inherit (final.nvidia-jetpack) cudaPackages; })
      (final: prev: { cudaPackages = prev.cudaPackages_12_6; })
    ];

    # Add cuda capabilities. Ensure cudaVersion and overlays match version from nvidia-smi
    nixpkgs.config = {
      allowUnfree = true;
      cudaSupport = true;
      cudaForwardCompat = true;
      cudaVersion = "12.6";
      cudaCapabilities = [ "8.7" ]; # For orin
    };
    hardware.graphics.enable = true; # Enable GPU for CUDA 

    # Display output is non-functional on the Orin AGX.
    # https://github.com/anduril/jetpack-nixos/issues/85
    services.xserver.enable = false;
    nova.desktop.enable = false;
  };
}
