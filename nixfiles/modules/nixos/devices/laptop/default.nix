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
      enableAllHardware = lib.mkDefault true;
      enableAllFirmware = lib.mkDefault true;
    };

    # Detect other OS on boot
    boot.loader = {
      efi.canTouchEfiVariables = lib.mkDefault true;
      systemd-boot.enable = lib.mkDefault true;
      timeout = lib.mkDefault 30;
    };

    nixpkgs = {
      # Allow non-free drivers
      config.allowUnfree = lib.mkDefault true;

      # Define platform as x86_64
      hostPlatform = lib.mkDefault "x86_64-linux";
    };
  };
}
