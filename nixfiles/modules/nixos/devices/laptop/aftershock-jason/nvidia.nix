{ config, lib, ... }:

let
  cfg = config.devices.laptop.aftershock-jason;
in
{
  # GTX 1050ti Mobile
  config = lib.mkIf cfg.enable {

    hardware.nvidia = {
      package = config.boot.kernelPackages.nvidiaPackages.legacy_535;
      prime = {
        intelBusId = "PCI:0:2:0";
        nvidiaBusId = "PCI:1:0:0"; 
        offload = {
          # The 1050 Ti does not seem to like PRIME offloading.
          enable = false;
          enableOffloadCmd = false;
        };
        sync.enable = true; # Render everything on the NVIDIA GPU to work around offloading issues.
      };
      open = false;
    };

    nixpkgs.config = {
      cudaCapabilities = [ "6.0" ];
    };

    # GDM seems to have weird issues on Wayland when the NVIDIA driver is enabled.
    # - The login screen restarts once after logging in
    # - VSCode(ium) hangs unless it is ran on the NVIDIA GPU
    nova.desktop.wayland.enable = false;
  };
}
