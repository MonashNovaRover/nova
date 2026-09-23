{ 
pkgs ? import <nixpkgs> {}
}:


pkgs.stdenv.mkDerivation rec{
  name = "gui";

  src = ./.;

  shellHook = ''
    realpath ${toString ../../../nova-gui}
  '';
}
