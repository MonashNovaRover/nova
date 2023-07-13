{ config, pkgs, lib, ... }:

let
  cfg = config.nova.branding;
in
{
  options.nova.branding.enable = lib.mkEnableOption "Nova Rover branding";

  config = lib.mkIf cfg.enable {
    boot = {
      initrd.systemd.enable = lib.mkDefault true;
      plymouth = {
        enable = true;
        logo = "${pkgs.nova.nova-icons}/share/icons/hicolor/128x128/apps/nova-logo-white.png";
      };
    };

    programs.dconf.profiles.gdm = lib.mkIf config.services.xserver.displayManager.gdm.enable (lib.mkForce (
      # Based on: https://github.com/NixOS/nixpkgs/blob/4bc72cae107788bf3f24f30db2e2f685c9298dc9/nixos/modules/services/x11/display-managers/gdm.nix
      # Can be cleaned up once https://github.com/NixOS/nixpkgs/pull/234615 or https://github.com/NixOS/nixpkgs/pull/189099 is merged.
      let
        customDconf = pkgs.writeTextFile {
          name = "gdm-dconf";
          destination = "/dconf/gdm-custom";
          text = ''
            [org/gnome/login-screen]
            logo='${pkgs.nova.nova-icons}/share/icons/hicolor/512x512/apps/nova-logo-white.png'
          '';
        };

        customDconfDb = pkgs.stdenv.mkDerivation {
          name = "gdm-dconf-db";
          buildCommand = ''
            ${pkgs.dconf}/bin/dconf compile $out ${customDconf}/dconf
          '';
        };
      in
      pkgs.stdenv.mkDerivation {
        name = "dconf-gdm-profile";
        buildCommand = ''
          # Check that the GDM profile starts with what we expect.
          if [ $(head -n 1 ${pkgs.gnome.gdm}/share/dconf/profile/gdm) != "user-db:user" ]; then
            echo "GDM dconf profile changed, please update gdm.nix"
            exit 1
          fi
          # Insert our custom DB behind it.
          sed '2ifile-db:${customDconfDb}' ${pkgs.gnome.gdm}/share/dconf/profile/gdm > $out
        '';
      }
    ));

    home-manager.nova.sharedModules = [{
      nova.branding.enable = true;
    }];
  };
}
