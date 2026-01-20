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

    # Apply global cuda overlays
    nixpkgs.overlays = [
      (final: _: { inherit (final.nvidia-jetpack) cudaPackages; })
    ];

    # Add cuda capabilities
    nixpkgs.config = {
      allowUnfree = true;
      cudaSupport = true;
      cudaCapabilities = [ "8.7" ]; # For orin
    };
    hardware.graphics.enable = true; # Enable GPU for CUDA
    
    # Add nvidia cachix
    nix.settings = {
      extra-experimental-features = [ "nix-command" "flakes" ];
      substituters = [
        "https://nix-community.cachix.org"
        "https://cuda-maintainers.cachix.org"
      ];
      trusted-public-keys = [
        "nix-community.cachix.org-1:mB9FSh9qf2dCimDSUo8Zy7bkq5CX+/rkCWyvRCYg3Fs="
        "cuda-maintainers.cachix.org-1:0dq3bujKpuEPMCX6U4WylrUDZ9JyUG0VpVZa7CNfq5E="
      ];
    };

    # Display output is non-functional on the Orin Nano.
    # https://github.com/anduril/jetpack-nixos/issues/85
    services.xserver.enable = false;
    nova.desktop.enable = false;
  };
}
