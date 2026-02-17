{ config, testScriptCommon, ... }:
let
  vmConf = {
    interfaces = {
      ethA = {
        vlan = 10;
        assignIP = false;
      };
      ethB = {
        vlan = 11;
        assignIP = false;
      };
    };
  };
in
{
  name = "networking";

  nodes = {
    rover = { config, pkgs, lib, ... }: {

      virtualisation = vmConf;

      nova.networking = {
        ethernetInterface = "ethA";
        wifiInterface = "none";
        secondaryEthernetInterface = "ethB";
        rover = {
          enable = true;
          ethernetIpAddr = "10.0.0.10";
          hostname = "rover";
        };
      };
    };

    base = { pkgs, ... }: {
      virtualisation = vmConf;

      nova.networking = {
        ethernetInterface = "ethA";
        wifiInterface = "none";
        secondaryEthernetInterface = "ethB";
        rover = {
          enable = true;
          ethernetIpAddr = "10.0.0.1";
          hostname = "base";
        };
      };
    };
  };

  enableOCR = false;

  #extraPythonPackages = ps: with ps; [ pyyaml types-pyyaml ];

  testScript = { ... }@args: ''
    ${testScriptCommon args}
  '' + builtins.readFile ./script.py;
}
