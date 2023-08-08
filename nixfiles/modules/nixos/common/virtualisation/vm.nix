{
  virtualisation.vmVariant = { modulesPath, lib, ... }: {
    imports = [ "${modulesPath}/installer/cd-dvd/channel.nix" ];

    # Persist store operations by default to reduce confusion.
    #
    # When the Nix store is cleaned on a reboot, its database is not updated
    # accordingly to remove the lost paths. This causes problems when trying to
    # use those paths after rebooting - Nix thinks they still exist, but they
    # do not.
    virtualisation.writableStoreUseTmpfs = lib.mkDefault false;

    services.xserver.enable = lib.mkDefault true;

    nova = {
      profile = lib.mkDefault "shared";
      substituters.nova.enable = lib.mkDefault false;
    };

    home-manager.nova.sharedModules = [{
      home.stateVersion = lib.mkDefault (lib.warn "home.stateVersion has not been set." lib.trivial.release);
    }];
  };
}
