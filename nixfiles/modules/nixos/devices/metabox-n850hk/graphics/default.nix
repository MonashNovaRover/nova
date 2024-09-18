{ config, lib, ... }:

{
  imports = [
    ./compute.nix
    ./video.nix
  ];

  config = lib.mkIf config.devices.metabox-n850hk.enable {
    hardware.nvidia = {
      modesetting.enable = true;
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
    };

    services.xserver.videoDrivers = [ "modesetting" "nvidia" ];

    services.switcherooControl.enable = true;

    # GDM seems to have weird issues on Wayland when the NVIDIA driver is enabled.
    # - The login screen restarts once after logging in
    # - VSCode(ium) hangs unless it is ran on the NVIDIA GPU
    nova.desktop.wayland.enable = false;
  };
}
