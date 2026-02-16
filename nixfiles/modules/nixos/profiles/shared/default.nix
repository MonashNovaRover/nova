# A configuration profile for shared team devices.

{ config, lib, ... }:

let
  profile = config.nova.profile;
in
{
  config = lib.mkIf (profile == "shared") {
    boot.kernel.sysctl = {
      "kernel.dmesg_restrict" = false;
    };

    environment.variables.EDITOR = "vim";

    security.polkit.extraConfig = ''
      polkit.addRule(function(action, subject) {
        // Lookup properties for manage-units are defined here:
        // https://github.com/systemd/systemd/blob/v254/src/core/dbus-util.c#L160
        if (action.id === "org.freedesktop.systemd1.manage-units" && subject.user === "nova") {
          return polkit.Result.YES;
        }
      });
    '';

    services.openssh = {
      enable = true;
      settings.X11Forwarding = true;
    };

    # For ghostty
    programs.ssh.extraConfig = ''
      Host *
        SetEnv TERM=xterm-256color
    '';

    # Most fingerprint hardware and software only supports 10 fingers.
    services.fprintd.enable = false;

    users.mutableUsers = false;

    nova = {
      users.nova.enable = true;
      branding.enable = true;
      desktop.enable = lib.mkDefault true;
    };

    peripherals.webcams.enable = true;
    peripherals.realsense.enable = false;
    peripherals.oak-d.enable = true;
    peripherals.hydraprobe.enable = true;
    peripherals.gps.enable = true;
  };
}
