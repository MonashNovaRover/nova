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

      environment.systemPackages = with pkgs; [
        tcpdump
      ];

      nova.networking = {
        ethernetInterface = "ethA";
        wifiInterface = "none";
        secondaryEthernetInterface = "ethB";
        prp = {
          enable = true;
          address = "0.10";
        };
      };
    };

    base = { pkgs, ... }: {
      virtualisation = vmConf;

      environment.systemPackages = with pkgs; [
        tcpdump
      ];

      nova.networking = {
        ethernetInterface = "ethA";
        wifiInterface = "none";
        secondaryEthernetInterface = "ethB";
        prp = {
          enable = true;
          address = "0.100";
        };
      };
    };
  };

  enableOCR = false;

  #extraPythonPackages = ps: with ps; [ pyyaml types-pyyaml ];

  testScript = { nodes, ... }@args: ''
    ${testScriptCommon args}
  '' + builtins.readFile ./script.py;
}
