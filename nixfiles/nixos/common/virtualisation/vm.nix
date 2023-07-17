{
  virtualisation.vmVariant = { lib, ... }: {
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
