{ config, testScriptCommon, ... }:

{
  name = "cameras-webrtc";

  nodes = {
    rover = { config, pkgs, lib, ... }: {
      virtualisation.memorySize = (6 + config.nova.mocking.cameras.count / 2) * 1024; # The camera streaming is quite memory intensive.

      nova.mocking.cameras = {
        enable = true;
        count = 1;
        firstNumber = 1;
        specs = {
          width = 1280;
          height = 720;
          framerate = 30;
        };
      };
    };

    base = { pkgs, ... }: {
      services.nova-gui.enable = true;
    };
  };

  enableOCR = true;

  extraPythonPackages = ps: with ps; [ pyyaml types-pyyaml ];

  testScript = { ... }@args: ''
    ${testScriptCommon args}
    camera_count = ${toString config.nodes.rover.nova.mocking.cameras.count};
  '' + builtins.readFile ./script.py;
}
