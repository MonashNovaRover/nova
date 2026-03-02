{ config, lib, ... }:

let
  cfg = config.devices.jetson.orin-nano;
in
{
  imports = [
    ./boot
  ];

  options.devices.jetson.orin-nano.enable = lib.mkEnableOption "configuration for the NVIDIA Jetson Orin Nano";

  config = lib.mkIf cfg.enable {
    devices.jetson.enable = true;
    hardware.nvidia-jetpack = {
      enable = true;
      som = "orin-nano";
      super = true; # Super speed
    };

    nova.networking = {
      wifiInterface = "wlP1p1s0";
      ethernetInterface = "enP8p1s0";
    };

    systemd.network = {
      links = {
        "20-can0" = {
          matchConfig = {
            Path = "platform-c310000.mttcan";
            Driver = "mttcan";
          };
          linkConfig = {
            Name = "can0";
          };
        };

        "20-can1" = {
          matchConfig = {
            Path = "platform-3210000.spi-cs-0";
            Driver = "mcp251xfd";
          };
          linkConfig = {
            Name = "can1";
          };
        };

        "20-can2" = {
          matchConfig = {
            Path = "platform-3230000.spi-cs-0";
            Driver = "mcp251xfd";
          };
          linkConfig = {
            Name = "can2";
          };
        };
      };
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

    # Display output is non-functional on the Orin Nano.
    # https://github.com/anduril/jetpack-nixos/issues/85
    services.xserver.enable = false;
    nova.desktop.enable = false;
  };
}
