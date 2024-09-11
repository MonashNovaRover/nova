{ config, lib, ... }:

{
  imports = [
    ./compute.nix
    ./video.nix
  ];

  config = lib.mkIf config.devices.metabox-v158pnh.enable {
    hardware.nvidia = {
      modesetting.enable = true;
      prime = {
        intelBusId = "PCI:0:2:0";
        nvidiaBusId = "PCI:1:0:0";
        offload = {
          enable = true;
          enableOffloadCmd = true;
        };
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
