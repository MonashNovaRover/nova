# A configuration profile for shared team devices.

{ config, lib, ... }:

let
  profile = config.nova.profile;
in
{
  config = lib.mkIf (profile == "shared") {
    services.openssh = {
      settings.X11Forwarding = true;
    };

    # For ghostty
    programs.ssh.extraConfig = ''
      Host *
        SetEnv TERM=xterm-256color
    '';

    # Most fingerprint hardware and software only supports 10 fingers.
    services.fprintd.enable = false;

    nova = {
      desktop.enable = lib.mkDefault true;
    };

    peripherals.webcams.enable = true;
    peripherals.realsense.enable = false;
    peripherals.oak-d.enable = true;
    peripherals.hydraprobe.enable = true;
    peripherals.gps.enable = true;
  };
}
