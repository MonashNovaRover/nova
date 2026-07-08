{ config, testScriptCommon, ... }:

{
  name = "can";

  nodes = {
    rover = { config, pkgs, lib, ... }: {
      virtualisation.cores = lib.mkForce 4; # ros2control is big

      nova.mocking.can = {
        enable = true;
      };


      environment.systemPackages = with pkgs; [
        jq can-utils iproute2
      ];
    };

    base = { pkgs, ... }: {
    };
  };


  extraPythonPackages = ps: with ps; [ pyyaml types-pyyaml ];

  testScript = { nodes, ... }@args: ''
    ${testScriptCommon args}
  '' + builtins.readFile ./script.py;
}
