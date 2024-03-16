{ config, testScriptCommon, ... }:

{
  name = "cameras-webrtc";

  nodes = {
    rover = { config, pkgs, lib, ... }: {
      virtualisation.memorySize = (6 + config.nova.mocking.cameras.count / 2) * 1024; # The camera streaming is quite memory intensive.

      nova.mocking.cameras = {
        enable = true;
        count = 4;
        firstNumber = 1;
        specs = {
          width = 1280;
          height = 720;
          framerate = 30;
        };
      };

      environment.systemPackages = with pkgs; [
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
    camera_count = ${toString config.nodes.rover.nova.mocking.cameras.count};
  '' + builtins.readFile ./script.py;
}
