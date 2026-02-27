# Configuration shared between many profiles.

{ config, lib, ... }:

let
  profile = config.nova.profile;
in
{
  config = lib.mkIf (profile == "shared" || profile == "mast") {
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
    };

    users.mutableUsers = false;

    nova = {
      users.nova.enable = true;
      branding.enable = true;
    };
  };
}
