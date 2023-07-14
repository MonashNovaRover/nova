{ supportedSystems ? [ "x86_64-linux" "aarch64-linux" ]
, nixpkgs
, src
, declInput
, ...
}@args:

let
  novaRepos = import ./nova-repos.nix;
  pkgs = import nixpkgs { };

  mkJobset = { description, nixexprpath, inputs ? { }, ... }@args: {
    enabled = 1;
    hidden = false;
    inherit description;
    nixexprinput = "src";
    inherit nixexprpath;
    checkinterval = 60;
    schedulingshares = 100;
    enableemail = false;
    enable_dynamic_run_command = false;
    emailoverride = "";
    keepnr = 3;
  } // args // {
    inputs = {
      nixpkgs = mkGitHubInput { owner = "NixOS"; repo = "nixpkgs"; branch = "nixos-unstable"; };
      src = mkNovaInput { repo = "nixfiles"; };
      supportedSystems = {
        type = "nix";
        value = "[ ''${builtins.concatStringsSep "'' ''" supportedSystems}'' ]";
        emailresponsible = false;
      };
    } // inputs;
  };

  mkGitHubInput = { owner, repo, branch ? null }: {
    type = "git";
    value = "git@github.com:${owner}/${repo}.git${pkgs.lib.optionalString (branch != null) (" ${branch}")}";
    emailresponsible = false;
  };

  mkNovaInput = args: mkGitHubInput ({ owner = "MonashNovaRover"; } // args);

  novaInputs = builtins.mapAttrs
    (repo: branch: mkNovaInput { inherit repo branch; })
    novaRepos;

  novaPrs = builtins.foldl'
    (allPrs: repo:
      let
        branch = novaRepos.${repo};
        repoPrs = builtins.attrValues (builtins.fromJSON (builtins.readFile args."${repo}-pr-json"));
      in
      allPrs ++ builtins.filter
        (pr:
          # We're only interested in open PRs.
          pr.state == "open" &&

          # Ensure that the PR is targeting the expected branch, or the default branch if none is specified.
          (pr.base.ref == (if branch == null then pr.base.repo.default_branch else branch)))
        repoPrs)
    [ ]
    (builtins.attrNames novaRepos);

  mkWorkspaceJobset = pr: mkJobset {
    # If a PR is given, make a oneshot jobset.
    # When the PR is updated, its commit SHA will change, and the jobset will
    # change and be re-evaluated - so there's no need for scheduling.
    # These magic numbers come from https://github.com/NixOS/hydra/blob/526e8bd7441d1beb271ff89bbca3604077ecffdb/src/hydra-evaluator/hydra-evaluator.cc#L142.
    enabled = if pr == null then 1 else 2;
    description = "Nova Rover workspaces${pkgs.lib.optionalString (pr != null) (" - ${pr.base.repo.name}#${toString pr.number} (${pr.title})")}";
    nixexprpath = "ci/jobsets/workspaces.nix";
    inputs = novaInputs // (pkgs.lib.optionalAttrs (pr != null) {
      ${pr.base.repo.name} = mkGitHubInput {
        owner = pr.head.repo.owner.login;
        repo = pr.head.repo.name;
        branch = pr.head.sha;
      };
    });
  };

  jobsets = {
    isos = mkJobset {
      description = "Nova Rover ISOs";
      nixexprpath = "ci/jobsets/isos.nix";
      inputs = novaInputs // {
        home-manager = mkGitHubInput {
          owner = "nix-community";
          repo = "home-manager";
        };
      };
    };
    workspaces = mkWorkspaceJobset null;
  } // builtins.listToAttrs
    (map
      (pr:
        pkgs.lib.nameValuePair
          "workspaces-pr-${pr.base.repo.name}-${toString pr.number}"
          (mkWorkspaceJobset pr))
      novaPrs);
in
{
  jobsets =
    pkgs.writeText "jobset.json" (builtins.toJSON jobsets);
}
