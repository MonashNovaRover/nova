let
  revisions = builtins.fromJSON (builtins.readFile ../../revisions.json);
  jetpack-nixos = builtins.fetchTarball {
    url = "https://github.com/anduril/jetpack-nixos/archive/${revisions.jetpack-nixos.rev}.tar.gz";
    sha256 = revisions.jetpack-nixos.hash;
  };
  jetpack-nixos-module = (import (builtins.toPath "${jetpack-nixos}/modules/default.nix") (import ( builtins.toPath "${jetpack-nixos}/overlay.nix")));
in
  jetpack-nixos-module
