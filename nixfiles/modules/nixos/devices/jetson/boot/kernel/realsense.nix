{ config, pkgs, lib, ... }:

let
  cfg = config.devices.jetson;
  librealsense = pkgs.fetchFromGitHub {
    owner = "realsenseai";
    repo = "librealsense";
    rev = "9a22c6001db6dc23d5e2c9098e9536a0b1bb4536";
    hash = "sha256-sjrOCvuKl+PpsKc74pLeRJY3z303Be62P1Q3L8ofQh8=";
  };

  # https://github.com/realsenseai/librealsense/blob/9a22c6001db6dc23d5e2c9098e9536a0b1bb4536/scripts/patch-realsense-ubuntu-L4T.sh#L189
  patchesRev = "6.0";
in
{
  config = lib.mkIf cfg.enable {
    boot.kernelPatches = [
      {
        name = "realsense-config-L4T";
        patch = null;
        extraConfig = ''
          HID_SENSOR_HUB m
          HID_SENSOR_ACCEL_3D m
          HID_SENSOR_GYRO_3D m
          HID_SENSOR_IIO_COMMON m
          HID_SENSOR_IIO_TRIGGER m
        '';
      }
      (lib.mkIf (patchesRev != "6.0") { # if not < 6.0
        name = "realsense-camera-formats-L4T";
        patch = "${librealsense}/scripts/Tegra/LRS_Patches/01-realsense-camera-formats-L4T-${patchesRev}.patch";
      })
      (lib.mkIf (patchesRev != "6.0") { # if not < 6.0
        name = "realsense-metadata-L4T";
        patch = "${librealsense}/scripts/Tegra/LRS_Patches/02-realsense-metadata-L4T-${patchesRev}.patch";
      })
      (lib.mkIf (patchesRev == "4.4") {
        name = "realsense-hid-L4T";
        patch = "${librealsense}/scripts/Tegra/LRS_Patches/03-realsense-hid-L4T-4.9.patch";
      })
      (lib.mkIf (patchesRev != "5.0.2" && patchesRev != "6.0") {
        name = "media-uvcvideo-mark-buffer-error-where-overflow";
        patch = "${librealsense}/scripts/Tegra/LRS_Patches/04-media-uvcvideo-mark-buffer-error-where-overflow.patch";
      })
      {
        name = "realsense-powerlinefrequency-control-fix";
        patch = "${librealsense}/scripts/Tegra/LRS_Patches/05-realsense-powerlinefrequency-control-fix.patch";
      }
    ];
  };
}
