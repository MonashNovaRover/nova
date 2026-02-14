{ ... }@args:

let
  lib = import ../lib.nix args;

  packageJobs = lib.novaForAllSystems (nova: nova.tests);
in
packageJobs
