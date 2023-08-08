{ lib, ... }:

{
  nixpkgs = {
    config.allowUnfree = true;
    overlays = lib.mkBefore [ (self: super: { nova = (import ../../.. { pkgs = self; }).pkgs; }) ];
  };
}
