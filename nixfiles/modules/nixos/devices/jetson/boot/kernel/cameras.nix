{ config, pkgs, lib, ... }:
let
  cfg = config.devices.jetson;
in
{
  config = lib.mkIf cfg.enable {
    boot.kernelPatches = [
      {
        name = "mjpeg bandwidth quirk";
        patch = ./patches/0001-uvcvideo-extend-UVC_QUIRK_FIX_BANDWIDTH-to-MJPEG-str.patch;
      }
      {
        name = "lifecams quirk";
        patch = ./patches/0003-uvcvideo-Add-FIX_BANDWIDTH-quirk-to-Microsoft-Lifeca.patch;
      }
      {
        name = "theta360cam video streaming";
        patch = ./patches/0002-uvcvideo-h264-for-thetacam.patch;
      }
    ];
  };
}

