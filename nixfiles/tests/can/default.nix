{ config, testScriptCommon, ... }:

{
  name = "can";

  nodes = {
    rover = { config, pkgs, lib, ... }: {
      nova.mocking.can = {
        enable = true;
      };

      systemd.network = {
        enable = true;
        netdevs = {
          vcan0 = {
            Kind = "vcan";
            Name = "can0";
          };
          vcan1 = {
            Kind = "vcan";
            Name = "can1";
          };
          vcan2 = {
            Kind = "vcan";
            Name = "can2";
          };
        };
      };
    };

    base = { pkgs, ... }: {
    };
  };


  extraPythonPackages = ps: with ps; [ pyyaml types-pyyaml ];

  testScript = { ... }@args: ''
    ${testScriptCommon args}
  '' + builtins.readFile ./script.py;
}
