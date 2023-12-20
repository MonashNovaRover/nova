{ hostPkgs, novaPkgs }:

let
  nixos-lib = import (hostPkgs.path + "/nixos/lib") { };

  runTest = module: nixos-lib.runTest {
    imports = [ module ];

    inherit hostPkgs;
    node.pkgs = novaPkgs;
    node.pkgsReadOnly = false;

    nodes =
      let
        novaCommon = { config, lib, ... }: {
          imports = [ ../nixos ];

          # Use the shared profile, as all team devices do.
          nova.profile = "shared";

          # Disable the Nova substituter to avoid providing a password.
          # There is no need for substitutions in the VM anyway: Everything is
          # built beforehand by the host.
          nova.substituters.nova.enable = false;

          # Many integration tests will not want everything to run automatically.
          nova.workspace.enable = lib.mkDefault false;
          environment.systemPackages = [ config.nova.workspace.package ];

          # Disable the desktop by default.
          nova.desktop.enable = lib.mkOverride 900 false;

          # Enable workspace shell completions.
          environment.interactiveShellInit = ''
            eval "$(mk-workspace-shell-setup)"
          '';

          # Log in automatically.
          services.getty.autologinUser = config.users.users.nova.name;
          services.xserver.displayManager.autoLogin = {
            enable = true;
            user = config.users.users.nova.name;
          };

          # The tests do not typically keep state, so their state version can
          # always be the latest.
          home-manager.users.nova.home.stateVersion = lib.mkDefault lib.trivial.release;
        };
      in
      {
        rover = {
          imports = [ novaCommon ];

          virtualisation = {
            cores = 2;
          };
        };

        base = {
          imports = [ novaCommon ];

          virtualisation = {
            memorySize = 4 * 1024;
            cores = 2;
          };

          services.xserver.enable = true;
          nova.desktop.enable = true;
        };
      };
  };
in
{ }
