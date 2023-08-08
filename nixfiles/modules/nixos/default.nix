{ pkgs, lib, ... }:

{
  imports = [
    <home-manager/nixos>
    ./common
    ./profiles
  ];

  nixpkgs = {
    config.allowUnfree = true;
    overlays = lib.mkBefore [ (self: super: { nova = (import ../../. { pkgs = self; }).pkgs; }) ];
  };

  home-manager.nova.sharedModules = [ ../home ];
}
