let
  # Don't know how to not do the absolute path here. nixos-build specifically seems to copy this folder to
  # the store before evaluating it so we can't use ../../../revisions.json as that ends up as
  # /nix/store/xxx/../../../revisions.json. Maybe flakes will make a better solution...
  revisions = builtins.fromJSON (builtins.readFile /home/nova/nova/nixfiles/revisions.json);
  jetpack-nixos = builtins.fetchTarball {
    url = "https://github.com/anduril/jetpack-nixos/archive/${revisions.jetpack-nixos.rev}.tar.gz";
    sha256 = revisions.jetpack-nixos.hash;
  };
  jetpack-nixos-module = (import (builtins.toPath "${jetpack-nixos}/modules/default.nix") (import ( builtins.toPath "${jetpack-nixos}/overlay.nix")));
in
  jetpack-nixos-module
