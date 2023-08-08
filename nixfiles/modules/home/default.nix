{ pkgs, lib, ... }:

{
  imports = [
    ./branding
    ./desktop
    ./nova
    ./workspace
  ];

  nixpkgs = {
    config.allowUnfree = true;
    overlays = lib.mkBefore [ (self: super: { nova = (import ../../. { pkgs = self; }).pkgs; }) ];
  };
}
