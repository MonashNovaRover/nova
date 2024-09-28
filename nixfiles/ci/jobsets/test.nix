{ supportedSystems
, nixpkgs
, nova-monorepo
, rosDistro
, ...
}@args:

let
  nixfiles = nova-monorepo + "/nixfiles";
  lib = import ../lib.nix args;
in
  throw nova-monorepo