{ nixpkgs
, ...
}@args:

let
  pkgs = import nixpkgs { };
  revisions = builtins.fromJSON (builtins.readFile ../../revisions.json);
  allNovaRepos = builtins.foldl' pkgs.lib.recursiveUpdate { } (builtins.attrValues (import ../nova-repos.nix));
in
rec {
  mkGitHubInput = { owner, repo, branch ? null }: {
    type = "git";
    value = "git@github.com:${owner}/${repo}.git${pkgs.lib.optionalString (branch != null) (" ${branch}")}";
    emailresponsible = false;
  };

  mkNovaInput = args: mkGitHubInput ({ owner = "MonashNovaRover"; } // args);

  novaInputs = builtins.mapAttrs
    (repo: branch: mkNovaInput { inherit repo branch; })
    allNovaRepos;

  homeManagerInput = mkGitHubInput {
    owner = "nix-community";
    repo = "home-manager";
  };

  jetpackNixosInput = mkGitHubInput {
    owner = "anduril";
    repo = "jetpack-nixos";
    branch = revisions.jetpack-nixos.rev; # Pin this as they have broken it before.
  };

  nixosHardwareInput = mkGitHubInput {
    owner = "NixOS";
    repo = "nixos-hardware";
  };
}
