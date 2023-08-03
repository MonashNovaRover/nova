{ supportedSystems ? [ "x86_64-linux" "aarch64-linux" ]
, nixpkgs
, src
, declInput
, ...
}@args:

let
  pkgs = import nixpkgs { };
  allNovaRepos = builtins.foldl' pkgs.lib.recursiveUpdate { } (builtins.attrValues (import ./nova-repos.nix));

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
    keepnr = 1;
  } // args // {
    inputs = {
      nixpkgs = mkGitHubInput { owner = "NixOS"; repo = "nixpkgs"; branch = "nixos-unstable"; };
      src = mkNovaInput { repo = "nixfiles"; };
      supportedSystems = {
        type = "nix";
        value = "[ \"${builtins.concatStringsSep "\" \"" supportedSystems}\" ]";
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
    allNovaRepos;

  novaPrs = builtins.foldl'
    (allPrs: repo:
      let
        branch = allNovaRepos.${repo};
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
    (builtins.attrNames allNovaRepos);

  homeManagerInput = mkGitHubInput {
    owner = "nix-community";
    repo = "home-manager";
  };

  mkWorkspaceJobset = rosDistro: pr: mkJobset {
    description =
      let
        distroTag = pkgs.lib.optionalString (rosDistro != null) " (for ${pkgs.lib.toUpper (builtins.substring 0 1 rosDistro)}${builtins.substring 1 (builtins.stringLength rosDistro) rosDistro})";
        prTag = pkgs.lib.optionalString (pr != null) (" - ${pr.base.repo.name}#${toString pr.number} (${pr.title})");
      in
      "Nova Rover workspaces${distroTag}${prTag}";
    nixexprpath = "ci/jobsets/workspaces.nix";
    inputs = novaInputs //
      {
        rosDistro = {
          type = "nix";
          value = "${if rosDistro == null then "null" else "\"${rosDistro}\""}";
          emailresponsible = false;
        };
      } //
      (pkgs.lib.optionalAttrs (pr != null) {
        ${pr.base.repo.name} = mkGitHubInput {
          owner = pr.head.repo.owner.login;
          repo = pr.head.repo.name;
          branch = pr.head.ref;
        };
      });
  };

  mkWorkspaceDistroJobsets = rosDistro:
    let
      baseName = "workspaces";
      distroTag = pkgs.lib.optionalString (rosDistro != null) "-${rosDistro}";
    in
    {
      "${baseName}${distroTag}" = mkWorkspaceJobset rosDistro null;
    } // builtins.listToAttrs
      (map
        (pr: pkgs.lib.nameValuePair
          "workspaces${distroTag}-pr-${pr.base.repo.name}-${toString pr.number}"
          (mkWorkspaceJobset rosDistro pr))
        novaPrs);

  mkAllWorkspaceJobsets = extraDistros: builtins.foldl'
    (jobs: distro: jobs // mkWorkspaceDistroJobsets distro)
    (mkWorkspaceDistroJobsets null)
    extraDistros;

  jobsets = mkAllWorkspaceJobsets [ "humble" ] // {
    docs = mkJobset {
      description = "Nova Rover documentation";
      nixexprpath = "ci/jobsets/docs.nix";
      inputs = { home-manager = homeManagerInput; };
    };
    isos = mkJobset {
      description = "Nova Rover ISOs";
      nixexprpath = "ci/jobsets/isos.nix";
      inputs = novaInputs // {
        nixpkgs-stable = mkGitHubInput { owner = "NixOS"; repo = "nixpkgs"; branch = "nixos-23.05"; };
        home-manager = homeManagerInput;
      };
      checkinterval = 60 * 60 * 24 * 7;
    };
  };
in
{
  jobsets =
    pkgs.writeText "jobset.json" (builtins.toJSON jobsets);
}
