{ config, lib, ... }:

let
  cfg = config.devices.laptop;
in
{
  imports = [
    # Laptops
    ./aftershock-heavy
    ./aftershock-jason
    ./aftershock-light
    ./aftershock-pocketrocket
    ./gigabyte
    ./metabox-new
    ./metabox-old

    # Config files
    ./amd.nix
    ./intel.nix
    ./nvidia.nix
    ./performance.nix
  ];

  options.devices.laptop.enable = lib.mkEnableOption "Nova Rover base station laptop configuration";

  config = lib.mkIf cfg.enable {

    # Enable everything
    hardware = {
      enableAllHardware = true;
      enableAllFirmware = true;
    };

    # Detect other OS on boot
    boot.loader = {
      efi.canTouchEfiVariables = true;
      systemd-boot.enable = true;
      timeout = 30;
    };

    nixpkgs = {
      # Allow non-free drivers
      config.allowUnfree = true;

      # Define platform as x86_64
      hostPlatform = "x86_64-linux";
    };
  };
}
