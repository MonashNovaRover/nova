{ testScriptCommon, ... }:

let
  cameraCount = 4;
in
{
  name = "cameras-webrtc";

  nodes = {
    rover = { config, pkgs, lib, ... }: {
      virtualisation.memorySize = (6 + cameraCount / 2) * 1024; # The camera streaming is quite memory intensive.

      boot = {
        extraModulePackages = with config.boot.kernelPackages; [ v4l2loopback ];
        kernelModules = [ "v4l2loopback" ];
        extraModprobeConfig = "options v4l2loopback exclusive_caps=${builtins.concatStringsSep "," (map toString (lib.replicate cameraCount 1))} video_nr=${builtins.concatStringsSep "," (map toString (lib.range 1 cameraCount))}";
      };

      services.udev.extraRules = ''
        SUBSYSTEM=="video4linux", ATTR{max_openers}=="?*", ENV{ID_SERIAL}="camera$number", ENV{ID_PATH}="platform-6682000.xhci-usb-0:1.0:1:$number.0"
      '';

      environment.systemPackages = with pkgs; [
        # For some strange reason, the GStreamer package defaults to the bin
        # output, which does not contain any of the core element libraries.
        gst_all_1.gstreamer.bin
        gst_all_1.gstreamer.out
        gst_all_1.gst-plugins-base
        gst_all_1.gst-plugins-good

        (writeScriptBin "gst-webrtc-ui-server" "${python3}/bin/python -m http.server -d '${./www}'")
      ];
    };

    base = { pkgs, ... }: {
      home-manager.users.nova = { options, ... }: {
        programs.firefox = {
          enable = true;
          package = options.programs.firefox.package.default.override {
            extraPrefs = ''
              // Prevent dialogs from interrupting the test flow
              lockPref("browser.shell.checkDefaultBrowser", false)

              // Allow autoplay
              // https://developer.mozilla.org/en-US/docs/Web/Media/Autoplay_guide#media.autoplay.default
              lockPref("media.autoplay.default", 0)
            '';
          };
        };
      };
    };
  };

  enableOCR = true;

  extraPythonPackages = ps: with ps; [ pyyaml types-pyyaml ];

  testScript = { ... }@args: ''
    ${testScriptCommon args}
    camera_count = ${toString cameraCount};
  '' + builtins.readFile ./script.py;
}
