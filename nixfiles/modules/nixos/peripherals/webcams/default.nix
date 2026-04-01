{ config, pkgs, lib, ... }:

let
  cfg = config.peripherals.webcams;
in
{
  options.peripherals.webcams.enable = lib.mkEnableOption "configuration for rover webcams";

  config = lib.mkIf cfg.enable {
    # https://github.com/leighleighleigh/novacarrier-tx2-image-builder/wiki/%5BFIX%5D-Many-USB-cameras-(Microsoft-LifeCam)
    boot.extraModprobeConfig = ''
      options uvcvideo \
        nodrop=1 \
        timeout=5000 \
        mjpeg_bpp=1 \
        quirks=${toString (builtins.foldl' builtins.bitOr 0 [
          128 # UVC_QUIRK_FIX_BANDWIDTH
        ])}

      options usbcore \
        autosuspend=-1 \
        usbfs_memory_mb=1000
    '';
  };
}
