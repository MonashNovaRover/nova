{ config, pkgs, lib, ... }:

let
  cfg = config.devices.jetson;

  # https://github.com/IntelRealSense/librealsense/blob/v2.54.1/scripts/patch-realsense-ubuntu-L4T.sh#L64
  patchesRev = "6.0";
in
{
  config = lib.mkIf cfg.enable {
    boot.kernelPatches = [
      {
        name = "realsense-camera-formats-L4T";
        patch = "${pkgs.nova.ros.realsense-patches}/01-realsense-camera-formats-L4T-${patchesRev}.patch";
        extraConfig = ''
          HID_SENSOR_HUB m
          HID_SENSOR_ACCEL_3D m
          HID_SENSOR_GYRO_3D m
          HID_SENSOR_IIO_COMMON m
          HID_SENSOR_IIO_TRIGGER m
        '';
      }
      {
        name = "realsense-metadata-L4T";
        patch = "${pkgs.nova.ros.realsense-patches}/02-realsense-metadata-L4T-${patchesRev}.patch";
      }
      (lib.mkIf (patchesRev == "4.4") {
        name = "realsense-hid-L4T";
        patch = "${pkgs.nova.ros.realsense-patches}/03-realsense-hid-L4T-4.9.patch";
      })
      (lib.mkIf (patchesRev != "5.0.2") {
        name = "media-uvcvideo-mark-buffer-error-where-overflow";
        patch = "${pkgs.nova.ros.realsense-patches}/04-media-uvcvideo-mark-buffer-error-where-overflow.patch";
      })
      {
        name = "realsense-powerlinefrequency-control-fix";
        patch = "${pkgs.nova.ros.realsense-patches}/05-realsense-powerlinefrequency-control-fix.patch";
      }
    ];
  };
}
