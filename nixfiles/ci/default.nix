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

  homeManagerInput = mkGitHubInput {
    owner = "nix-community";
    repo = "home-manager";
  };

  mkWorkspaceJobset = pr: mkJobset {
    description = "Nova Rover workspaces${pkgs.lib.optionalString (pr != null) (" - ${pr.base.repo.name}#${toString pr.number} (${pr.title})")}";
    nixexprpath = "ci/jobsets/workspaces.nix";
    inputs = novaInputs // (pkgs.lib.optionalAttrs (pr != null) {
      ${pr.base.repo.name} = mkGitHubInput {
        owner = pr.head.repo.owner.login;
        repo = pr.head.repo.name;
        branch = pr.head.ref;
      };
    });
  };

  jobsets = {
    docs = mkJobset {
      description = "Nova Rover documentation";
      nixexprpath = "ci/jobsets/docs.nix";
      inputs = { home-manager = homeManagerInput; };
    };
    isos = mkJobset {
      description = "Nova Rover ISOs";
      nixexprpath = "ci/jobsets/isos.nix";
      inputs = novaInputs // { home-manager = homeManagerInput; };
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
