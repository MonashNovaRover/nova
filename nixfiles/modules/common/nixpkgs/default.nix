{ config, lib, ... }:

{
  nixpkgs = {
    config.allowUnfree = true;
    overlays = lib.mkBefore [
      (self: super: {
        nova = (import ../../.. ({ pkgs = self; } // lib.optionalAttrs (config.nova.repos != null) {
          inherit (config.nova) repos;
        })).pkgs;
      })
    ];
  };
}
