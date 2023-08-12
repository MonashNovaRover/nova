{ config, pkgs, lib, ... }:

let
  cfg = config.nova.macros;
in
{
  options.nova.macros = {
    enable = lib.mkEnableOption "team shell macros";
    sourceDir = lib.mkOption {
      type = with lib.types; str;
      default = "~/src";
      description = "The source code directory.";
    };
    nixfileDir = lib.mkOption {
      type = with lib.types; str;
      default = "${cfg.sourceDir}/../nixfiles";
      description = "The directory containing the Nix files.";
    };
  };

  config = lib.mkIf cfg.enable {
    home = {
      shellAliases = rec {
        # Nix CLI shortcuts
        nova-build = "nix-build ${cfg.nixfileDir}";
        nova-shell = "nix-shell ${cfg.nixfileDir}";
        ws-build = "${nova-build} -A pkgs.ros.nova-workspace";
        ws-shell = "${nova-shell} -A pkgs.ros.nova-workspace.env";

        # Directory aliases
        nova = "cd ${cfg.sourceDir}";
        rover = "${nova}/ros/rover";
        core = "${nova}/ros/rover/core";
        control = "${nova}/ros/rover/control";
        electronics = "${nova}/ros/rover/electronics";
        elec = electronics;
        visualisation = "${nova}/ros/visualisation";
        visualization = visualisation;
        vis = visualisation;
        viz = vis;
        science = "${nova}/ros/rover/science";
        camerasdir = "${nova}/ros/cameras2";
        autonomous = "${nova}/ros/rover/autonomous";
        auto = autonomous;
        gui = "${nova}/ros/gui";
        tutorials = "${nova}/ros/tutorials";
        pic = "${nova}/other/pics";
        pics = pic;
        arduino = "${nova}/other/arduinos";
        arduinos = arduino;
        ik = "${nova}/other/ik_machine";
        coms = "${nova}/other/coms_utils";

        # Networking aliases
        jetson = "ssh -Y nvidia@192.168.1.204";
        jetson_wifi = "ssh -Y nvidia@tegra-ubuntu";

        # Stubs to ease migration
        setup = ''echo 'The setup alias is no longer necessary. To try new changes, please use "ws-build" or "nixos-rebuild" instead.' #'';
        check = ''echo 'The check alias is no longer relevant. Use "journalctl -u <service>" instead.' #'';
        stop = ''echo 'The stop alias is no longer relevant. Use "systemctl stop <service>" instead.' #'';
        restart = ''echo 'The restart alias is no longer relevant. Use "systemctl restart <service>" instead.' #'';
        wifi = ''echo 'The wifi alias is no longer relevant. Use "nmtui" or "nmcli" instead.' #'';
      };

      packages = with pkgs.nova-scripts; [
        can
      ];
    };

    nixpkgs.overlays = [
      (self: super: with self; {
        nova-scripts = self.lib.makeScope self.newScope (novaSelf: {
          can = writeShellApplication {
            name = "can";
            runtimeInputs = [ kmod iproute2 ];
            text = builtins.readFile ./can.sh;
          };
        });
      })
    ];
  };
}
