{ config, lib, pkgs, ... }:

let
  cfg = config.devices.jetson.orin-nano;
  # The ch341 kernel module is not included in the default kernel configuration for the Orin Nano,
  # but is required for some USB serial devices. We build it as an out-of-tree module.
  kernel = config.boot.kernelPackages.kernel;
  ch341Module = pkgs.stdenv.mkDerivation {
    pname = "ch341-kernel-module";
    inherit (kernel) src version postPatch;

    nativeBuildInputs = kernel.moduleBuildDependencies or [];

    kernelVersion = kernel.modDirVersion;

    buildPhase = ''
      runHook preBuild

      mkdir -p ch341-out-of-tree
      cp drivers/usb/serial/ch341.c ch341-out-of-tree/
      cd ch341-out-of-tree

      printf '%s\n' 'obj-m += ch341.o' > Makefile

      make -C ${kernel.dev}/lib/modules/${kernel.modDirVersion}/build \
        M=$(pwd) \
        modules

      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall

      install -D ch341.ko \
        $out/lib/modules/${kernel.modDirVersion}/extra/ch341.ko

      runHook postInstall
    '';

    meta = {
      description = "CH341/CH340 USB serial kernel module";
      license = lib.licenses.gpl2Only;
      platforms = lib.platforms.linux;
    };
  };
in
{
  imports = [
    ./boot
  ];

  options.devices.jetson.orin-nano.enable = lib.mkEnableOption "configuration for the NVIDIA Jetson Orin Nano";

  config = lib.mkIf cfg.enable {
    devices.jetson.enable = true;
    
    boot.extraModulePackages = [ ch341Module ];
    boot.kernelModules = [ "ch341" ];
    
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
